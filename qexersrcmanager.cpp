#include "qexersrcmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QTextCodec>

#include "qexe.h"

#include <QDebug>
#define OUT qInfo().noquote().nospace()
#define HEX(n) "0x" << QString::number(n, 16).toUpper()

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
    QLinkedList<QExeRsrcEntryPtr> directories;
    QHash<QExeRsrcEntryPtr, quint32> directoryOffs;
    QHash<QExeRsrcEntryPtr, QList<qint64>> directoryRefs;
    QLinkedList<QExeRsrcEntryPtr> dataDescs;
    QHash<QExeRsrcEntryPtr, QList<qint64>> dataDescRefs;
    QLinkedList<QString> strings;
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
    OUT << "ROMP 0: Calculating section sizes";
    SectionSizes sizes = calculateSectionSizes(m_root);
    OUT << "Directory size: " << HEX(sizes.directorySize);
    OUT << "Data entry size: " << HEX(sizes.dataDescSize);
    OUT << "String table size: " << HEX(sizes.stringSize);
    OUT << "Data size: " << HEX(sizes.dataSize);
    QExeSectionPtr sec = exeDat->sectionManager()->createSection(QLatin1String(".rsrc"), sizes.totalSize());
    sec->characteristics = QExeSection::ContainsInitializedData | QExeSection::IsReadable;
    QBuffer buf(&sec->rawData);
    buf.open(QBuffer::WriteOnly);
    QDataStream ds(&buf);
    ds.setByteOrder(QDataStream::LittleEndian);
    OUT << "ROMP 1: Writing directories";
    // write root directory
    QLinkedList<QExeRsrcEntryPtr> rootDirsName, rootDirsID;
    writeDirectory(buf, ds, m_root, rootDirsName, rootDirsID, symTbl);
    // write subdirectories of root
    QLinkedList<QExeRsrcEntryPtr> subdirsName, subdirsID;
    QExeRsrcEntryPtr entry;
    foreach (entry, rootDirsName)
        writeDirectory(buf, ds, entry, subdirsName, subdirsID, symTbl);
    foreach (entry, rootDirsID)
        writeDirectory(buf, ds, entry, subdirsName, subdirsID, symTbl);
    // finally, write subdirs of subdirs of root
    QLinkedList<QExeRsrcEntryPtr> ignored;
    foreach (entry, subdirsName)
        writeDirectory(buf, ds, entry, ignored, ignored, symTbl);
    foreach (entry, subdirsID)
        writeDirectory(buf, ds, entry, ignored, ignored, symTbl);
    OUT << "ROMP 2: Writing symbols";
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
        OUT << "nameOff is " << HEX(nameOff);
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
        OUT << "dataOff is " << HEX(dataOff);
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
            sz.dataSize += static_cast<quint32>(entry->data.size());
        } else {
            SectionSizes other = calculateSectionSizes(entry, allocStr);
            sz += other;
        }
    }
    return sz;
}

void QExeRsrcManager::writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, QLinkedList<QExeRsrcEntryPtr> &subdirsName, QLinkedList<QExeRsrcEntryPtr> &subdirsID, QExeRsrcManager::SymbolTable &symTbl)
{
    OUT << " === Writing directory " << entryID(dir) << " (" << dir.data() << ") === ";
    symTbl.directories += dir;
    symTbl.directoryOffs[dir] = static_cast<quint32>(dst.pos());
    // write unimportant fields
    ds << dir->directoryMeta.characteristics;
    ds << dir->directoryMeta.timestamp;
    ds << dir->directoryMeta.version;
    // first romp to count name/ID entries
    QLinkedList<QExeRsrcEntryPtr> entriesName, entriesID;
    quint16 entryCountName = 0, entryCountID = 0;
    QExeRsrcEntryPtr entry;
    foreach (entry, dir->children()) {
        if (entry->name.isEmpty()) {
            entryCountID++;
            entriesID += entry;
        } else {
            entryCountName++;
            entriesName += entry;
        }
    }
    // write em out
    OUT << "Entries: " << HEX(entryCountName) << " named, " << HEX(entryCountID) << " ID'd";
    ds << entryCountName;
    ds << entryCountID;
    // second romp to actually write entries
    // make a subdir list to write *after* writing directory
    OUT << " --- Writing entries --- ";
    writeEntries(dst, ds, entriesName, subdirsName, subdirsID, symTbl);
    writeEntries(dst, ds, entriesID, subdirsName, subdirsID, symTbl);
}

void QExeRsrcManager::writeEntries(QBuffer &dst, QDataStream &ds, QLinkedList<QExeRsrcEntryPtr> entries, QLinkedList<QExeRsrcEntryPtr> &subdirsName, QLinkedList<QExeRsrcEntryPtr> &subdirsID, QExeRsrcManager::SymbolTable &symTbl)
{
    QExeRsrcEntryPtr entry;
    foreach (entry, entries) {
        OUT << "Writing entry " << entryID(entry) << " (" << entry.data() << ")";
        if (entry->name.isEmpty()) {
            ds << entry->id;
        } else {
            symTbl.strings += entry->name;
            symTbl.stringRefs[entry->name] += dst.pos();
            ds << ~hiMask;
        }
        if (entry->type() == QExeRsrcEntry::Data) {
            symTbl.dataDescs += entry;
            symTbl.dataDescRefs[entry] += dst.pos();
            ds << static_cast<quint32>(0);
        } else {
            symTbl.directoryRefs[entry] += dst.pos();
            if (entry->name.isEmpty())
                subdirsID += entry;
            else
                subdirsName += entry;
            ds << ~hiMask;
        }
    }
}

void QExeRsrcManager::writeSymbols(QBuffer &dst, QDataStream &ds, QExeRsrcManager::SectionSizes sizes, QExeRsrcManager::SymbolTable &symTbl, quint32 offset)
{
    OUT << " === Writing symbols === ";
    QLinkedListIterator<QExeRsrcEntryPtr> dirsI(symTbl.directories);
    // write directroy references
    OUT << " --- Directory references --- ";
    while (dirsI.hasNext()) {
        QExeRsrcEntryPtr entry = dirsI.next();
        OUT << "Writing references to directory " << entryID(entry) << " (" << entry.data() << ")";
        quint32 v = symTbl.directoryOffs[entry] | hiMask;
        qint64 pos;
        foreach (pos, symTbl.directoryRefs[entry]) {
            dst.seek(pos);
            ds << v;
        }
    }
    // write actual data, remember offsets
    OUT << " --- Data --- ";
    QMap<QExeRsrcEntryPtr, quint32> dataPtrs;
    dst.seek(sizes.directorySize + sizes.dataDescSize + sizes.stringSize);
    QLinkedListIterator<QExeRsrcEntryPtr> dataDescsI(symTbl.dataDescs);
    while (dataDescsI.hasNext()) {
        QExeRsrcEntryPtr dataDesc = dataDescsI.next();
        OUT << "Writing data for entry " << entryID(dataDesc) << " (" << dataDesc.data() << ")";
        dataPtrs[dataDesc] = static_cast<quint32>(dst.pos()) + offset;
        dst.write(dataDesc->data);
    }
    // write data descriptions and their references
    OUT << " --- Data descriptions --- ";
    dst.seek(sizes.directorySize);
    dataDescsI.toFront();
    while (dataDescsI.hasNext()) {
        QExeRsrcEntryPtr dataDesc = dataDescsI.next();
        OUT << "Writing data description for entry " << entryID(dataDesc) << " (" << dataDesc.data() << ")";
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
    OUT << " --- String table --- ";
    QTextEncoder *encoder = QTextCodec::codecForName("UTF-16LE")->makeEncoder(QTextCodec::IgnoreHeader);
    dst.seek(sizes.directorySize + sizes.dataDescSize);
    QLinkedListIterator<QString> stringsI = QLinkedListIterator<QString>(symTbl.strings);
    while (stringsI.hasNext()) {
        QString str = stringsI.next();
        OUT << "Writing string " << str;
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
    }
}
