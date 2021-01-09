#include "qexersrcmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QTextCodec>
#include <QDataStream>

#include "qexe.h"

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }

constexpr quint32 hiMask = 0x80000000;

quint32 QExeRsrcManager::headerSize() const
{
    // our "header" is actually the .rsrc section's header, and section headers are always 0x28 bytes
    return 0x28;
}

QExeRsrcEntryPtr QExeRsrcManager::root() const
{
    return m_root;
}

QList<QExeRsrcEntryPtr> QExeRsrcManager::entriesFromPath(const QString &path) const
{
    QString p = path;
    if (p.startsWith("/"))
        p = p.mid(1);
    return m_root->fromPath(p);
}

QExeRsrcManager::QExeRsrcManager(QExe *exeDat, QObject *parent) : QObject(parent)
{
    this->exeDat = exeDat;
    m_root = QExeRsrcEntryPtr(new QExeRsrcEntry(QExeRsrcEntry::Directory));
}

bool QExeRsrcManager::read(QExeSectionPtr sec, QExeErrorInfo *errinfo)
{
    QBuffer buf(&sec->rawData);
    buf.open(QBuffer::ReadOnly);
    QDataStream ds(&buf);
    ds.setByteOrder(QDataStream::LittleEndian);
    m_root->removeAllChildren();
    if (!readDirectory(buf, ds, m_root, sec->virtualAddr)) {
        SET_ERROR_INFO(BadRsrc_InvalidFormat)
        return false;
    }
    buf.close();
    return true;
}

class QExeRsrcManager::SectionSizes {
public:
    quint32 directorySize;
    quint32 dataDescSize;
    quint32 stringSize;
    quint32 dataSize;
    quint32 totalSize() {
        return directorySize + dataDescSize + stringSize + dataSize;
    }
    SectionSizes& operator+=(const SectionSizes &other) {
        directorySize += other.directorySize;
        dataDescSize += other.dataDescSize;
        stringSize += other.stringSize;
        dataSize += other.dataSize;
        return *this;
    }
};

class QExeRsrcManager::SymbolTable {
public:
    std::list<QExeRsrcEntryPtr> directories;
    QHash<QExeRsrcEntryPtr, quint32> directoryOffs;
    QHash<QExeRsrcEntryPtr, QList<qint64>> directoryRefs;
    std::list<QExeRsrcEntryPtr> dataDescs;
    QHash<QExeRsrcEntryPtr, QList<qint64>> dataDescRefs;
    std::list<QString> strings;
    QHash<QString, QList<qint64>> stringRefs;
};

inline QString entryID(QExeRsrcEntryPtr e) {
    if (e->name.isEmpty())
        return QString("0x%1").arg(QString::number(e->id, 16).toUpper());
    else
        return e->name;
}

void QExeRsrcManager::toSection()
{
    SymbolTable symTbl;
    SectionSizes sizes = calculateSectionSizes(m_root);
    quint32 size = QExe::alignForward(sizes.totalSize(), exeDat->optionalHeader()->sectionAlign);
    QExeSectionPtr sec = exeDat->sectionManager()->createSection(QLatin1String(".rsrc"), size);
    sec->characteristics = QExeSection::ContainsInitializedData | QExeSection::IsReadable;
    QBuffer buf(&sec->rawData);
    buf.open(QBuffer::WriteOnly);
    QDataStream ds(&buf);
    ds.setByteOrder(QDataStream::LittleEndian);
    // write root directory
    std::list<QExeRsrcEntryPtr> rootDirsName, rootDirsID;
    writeDirectory(buf, ds, m_root, rootDirsName, rootDirsID, symTbl);
    // write subdirectories of root
    std::list<QExeRsrcEntryPtr> subdirsName, subdirsID;
    QExeRsrcEntryPtr entry;
    foreach (entry, rootDirsName)
        writeDirectory(buf, ds, entry, subdirsName, subdirsID, symTbl);
    foreach (entry, rootDirsID)
        writeDirectory(buf, ds, entry, subdirsName, subdirsID, symTbl);
    // finally, write subdirs of subdirs of root
    std::list<QExeRsrcEntryPtr> ignored;
    foreach (entry, subdirsName)
        writeDirectory(buf, ds, entry, ignored, ignored, symTbl);
    foreach (entry, subdirsID)
        writeDirectory(buf, ds, entry, ignored, ignored, symTbl);
    writeSymbols(buf, ds, sizes, symTbl, sec->virtualAddr);
    buf.close();
}

bool QExeRsrcManager::readDirectory(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset) {
    ds >> dir->directoryMeta.characteristics;
    ds >> dir->directoryMeta.timestamp;
    ds >> dir->directoryMeta.version;
    quint16 entriesName, entriesID;
    ds >> entriesName;
    ds >> entriesID;

    quint16 entries = entriesName + entriesID;
    for (quint32 i = 0; i < entries; i++)
        if (!readEntry(src, ds, dir, offset)) {
            return false;
        }
    return true;
}

bool QExeRsrcManager::readEntry(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset)
{
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(QExeRsrcEntry::Data));

    quint32 nameOff;
    ds >> nameOff;
    quint32 dataOff;
    ds >> dataOff;

    // read name/ID
    qint64 prevPos = src.pos();
    if ((nameOff & hiMask) == 0)
        // id
        child->id = nameOff;
    else {
        // name
        nameOff &= ~hiMask;
        src.seek(nameOff);
        quint16 nameLen;
        ds >> nameLen;
        QByteArray nameBytes = src.read(nameLen * 2);
        child->name = QTextCodec::codecForName("UTF-16LE")->makeDecoder(QTextCodec::IgnoreHeader)->toUnicode(nameBytes);
        src.seek(prevPos);
    }
    // read data/directory
    prevPos = src.pos();
    if ((dataOff & hiMask) == 0) {
        // data
        src.seek(dataOff);
        quint32 dataPtr;
        ds >> dataPtr;
        quint32 dataSize;
        ds >> dataSize;
        ds >> child->dataMeta.codepage;
        ds >> child->dataMeta.reserved;
        src.seek(dataPtr - offset);
        child->data = src.read(dataSize);
    } else {
        // directory
        if (dir->depth() == 2)
            // max depth is 2
            return false;
        dataOff &= ~hiMask;
        child->m_type = QExeRsrcEntry::Directory;
        src.seek(dataOff);
        if (!readDirectory(src, ds, child, offset))
            return false;
    }
    src.seek(prevPos);
    return dir->addChild(child);
}

const quint32 rsrcDataAlign = 4; // .rsrc data is aligned to DWORD boundary

QExeRsrcManager::SectionSizes QExeRsrcManager::calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr)
{
    if (allocStr == nullptr)
        allocStr = new QStringList();
    SectionSizes sz;
    sz.directorySize = 0x10;
    sz.dataDescSize = 0;
    sz.stringSize = 0;
    sz.dataSize = 0;
    QExeRsrcEntryPtr entry;
    foreach (entry, root->children()) {
        sz.directorySize += 0x10;
        if (!entry->name.isEmpty() && !allocStr->contains(entry->name)) {
            allocStr->append(entry->name);
            sz.stringSize += 2 + static_cast<quint32>(entry->name.size()) * 2;
        }
        if (entry->type() == QExeRsrcEntry::Data) {
            sz.dataDescSize += 0x10;
            sz.dataSize += QExe::alignForward(static_cast<quint32>(entry->data.size()), rsrcDataAlign);
        } else {
            SectionSizes other = calculateSectionSizes(entry, allocStr);
            sz += other;
        }
    }
    return sz;
}

void QExeRsrcManager::writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, std::list<QExeRsrcEntryPtr> &subdirsName, std::list<QExeRsrcEntryPtr> &subdirsID, QExeRsrcManager::SymbolTable &symTbl)
{
    symTbl.directories.push_front(dir);
    symTbl.directoryOffs[dir] = static_cast<quint32>(dst.pos());
    // write unimportant fields
    ds << dir->directoryMeta.characteristics;
    ds << dir->directoryMeta.timestamp;
    ds << dir->directoryMeta.version;
    // first romp to count name/ID entries
    std::list<QExeRsrcEntryPtr> entriesName, entriesID;
    quint16 entryCountName = 0, entryCountID = 0;
    QExeRsrcEntryPtr entry;
    foreach (entry, dir->children()) {
        if (entry->name.isEmpty()) {
            entryCountID++;
            entriesID.push_front(entry);
        } else {
            entryCountName++;
            entriesName.push_front(entry);
        }
    }
    // write em out
    ds << entryCountName;
    ds << entryCountID;
    // second romp to actually write entries
    // make a subdir list to write *after* writing directory
    writeEntries(dst, ds, entriesName, subdirsName, subdirsID, symTbl);
    writeEntries(dst, ds, entriesID, subdirsName, subdirsID, symTbl);
}

void QExeRsrcManager::writeEntries(QBuffer &dst, QDataStream &ds, std::list<QExeRsrcEntryPtr> entries, std::list<QExeRsrcEntryPtr> &subdirsName, std::list<QExeRsrcEntryPtr> &subdirsID, QExeRsrcManager::SymbolTable &symTbl)
{
    QExeRsrcEntryPtr entry;
    foreach (entry, entries) {
        if (entry->name.isEmpty()) {
            ds << entry->id;
        } else {
            symTbl.strings.push_front(entry->name);
            symTbl.stringRefs[entry->name] += dst.pos();
            ds << hiMask;
        }
        if (entry->type() == QExeRsrcEntry::Data) {
            symTbl.dataDescs.push_front(entry);
            symTbl.dataDescRefs[entry] += dst.pos();
            ds << static_cast<quint32>(0);
        } else {
            symTbl.directoryRefs[entry] += dst.pos();
            if (entry->name.isEmpty())
                subdirsID.push_front(entry);
            else
                subdirsName.push_front(entry);
            ds << hiMask;
        }
    }
}

void QExeRsrcManager::writeSymbols(QBuffer &dst, QDataStream &ds, QExeRsrcManager::SectionSizes sizes, QExeRsrcManager::SymbolTable &symTbl, quint32 offset)
{
    // write directroy references
    QExeRsrcEntryPtr entry;
    foreach (entry, symTbl.directories) {
        quint32 v = symTbl.directoryOffs[entry] | hiMask;
        qint64 pos;
        foreach (pos, symTbl.directoryRefs[entry]) {
            dst.seek(pos);
            ds << v;
        }
    }
    // write actual data, remember offsets
    QMap<QExeRsrcEntryPtr, quint32> dataPtrs;
    dst.seek(sizes.directorySize + sizes.dataDescSize + sizes.stringSize);
    QExeRsrcEntryPtr dataDesc;
    foreach (dataDesc, symTbl.dataDescs) {
        dataPtrs[dataDesc] = static_cast<quint32>(dst.pos()) + offset;
        dst.write(dataDesc->data);
        dst.seek(QExe::alignForward(static_cast<quint32>(dst.pos()), rsrcDataAlign));
    }
    // write data descriptions and their references
    dst.seek(sizes.directorySize);
    foreach (dataDesc, symTbl.dataDescs) {
        quint32 prevPos = static_cast<quint32>(dst.pos());
        qint64 pos;
        foreach (pos, symTbl.dataDescRefs[dataDesc]) {
            dst.seek(pos);
            ds << prevPos;
        }
        dst.seek(prevPos);
        quint32 dataPtr = dataPtrs[dataDesc];
        ds << dataPtr;
        ds << static_cast<quint32>(dataDesc->data.size());
        ds << dataDesc->dataMeta.codepage;
        ds << dataDesc->dataMeta.reserved;
    }
    // write strings and their references
    QTextEncoder *encoder = QTextCodec::codecForName("UTF-16LE")->makeEncoder(QTextCodec::IgnoreHeader);
    dst.seek(sizes.directorySize + sizes.dataDescSize);
    QString str;
    foreach (str, symTbl.strings) {
        quint32 prevPos = static_cast<quint32>(dst.pos());
        quint32 v = prevPos | hiMask;
        qint64 pos;
        foreach (pos, symTbl.stringRefs[str]) {
            dst.seek(pos);
            ds << v;
        }
        dst.seek(prevPos);
        ds << static_cast<quint16>(str.size());
        dst.write(encoder->fromUnicode(str));
    };
}
