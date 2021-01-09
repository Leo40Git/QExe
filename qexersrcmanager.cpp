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

QExeRsrcManager::QExeRsrcManager(QObject *parent) : QObject(parent)
{
    m_root = QExeRsrcEntryPtr(new QExeRsrcEntry(QExeRsrcEntry::Directory));
}

QExeRsrcEntryPtr QExeRsrcManager::root() const
{
    return m_root;
}

void QExeRsrcManager::correctOffsets(QExeSectionPtr rsrcSec, const qint64 shift)
{
    QBuffer buf(&rsrcSec->rawData);
    buf.open(QBuffer::ReadWrite);
    QDataStream ds(&buf);
    ds.setByteOrder(QDataStream::LittleEndian);
    shiftDirectory(buf, ds, shift, 0);
    buf.close();
}

void QExeRsrcManager::shiftDirectory(QBuffer &buf, QDataStream &ds, const qint64 shift, const quint32 ptr)
{
    buf.seek(ptr + 12);
    quint16 entriesName, entriesID;
    ds >> entriesName;
    ds >> entriesID;
    const quint32 entries = entriesName + entriesID;

    quint32 results[entries];
    for (quint32 i = 0; i < entries; i++) {
        quint32 result;
        ds >> result; // throw this one away
        ds >> result;
        results[i] = result;
    }

    for (quint32 i = 0; i < entries; i++) {
        if ((results[i] & hiMask) != 0)
            shiftDirectory(buf, ds, shift, results[i] & ~hiMask);
        else {
            buf.seek(results[i]);
            quint32 val;
            ds >> val;
            buf.seek(results[i]);
            val += shift;
            ds << val;
        }
    }
}

bool QExeRsrcManager::addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, QExeSectionPtr sec)
{
    QExeSectionPtr rsrcSec = secMgr->removeSection(secMgr->rsrcSectionIndex());
    quint32 oldRVA = 0;
    if (!rsrcSec.isNull())
        oldRVA = rsrcSec->virtualAddr;
    if (!secMgr->addSection(sec)) {
        secMgr->addSection(rsrcSec);
        return false;
    }
    if (!rsrcSec.isNull()) {
        if (secMgr->addSection(rsrcSec))
            // this can only fail if .rsrc already exists
            // since we removed it earlier, assume the user just added their own .rsrc section
            // and don't bother correcting the "old" one
            correctOffsets(rsrcSec, rsrcSec->virtualAddr - oldRVA);
    }
    return true;
}

bool QExeRsrcManager::addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars)
{
    return addBeforeRsrcSection(secMgr, QExeSectionPtr(new QExeSection(name, data, chars)));
}

bool QExeRsrcManager::addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, quint32 size, QExeSection::Characteristics chars)
{
    return addBeforeRsrcSection(secMgr, QExeSectionPtr(new QExeSection(name, size, chars)));
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

class QExeRsrcManager::SubdirStorage {
public:
    std::list<QExeRsrcEntryPtr> id;
    std::list<QExeRsrcEntryPtr> name;

    void add(QExeRsrcEntryPtr subdir) {
        if (subdir->name.isEmpty())
            id.push_front(subdir);
        else
            name.push_front(subdir);
    }

    bool empty() {
        return id.empty() && name.empty();
    }

    void move(SubdirStorage &from) {
        id = from.id;
        from.id.clear();
        name = from.name;
        from.name.clear();
    }
};

QExeSectionPtr QExeRsrcManager::toSection(quint32 sectionAlign)
{
    SymbolTable symTbl;
    SectionSizes sizes = calculateSectionSizes(m_root);
    quint32 size = QExe::alignForward(sizes.totalSize(), sectionAlign);
    QExeSectionPtr sec = QExeSectionPtr(new QExeSection(QLatin1String(".rsrc"), size,
                                                        QExeSection::ContainsInitializedData | QExeSection::IsReadable));

    QBuffer dst(&sec->rawData);
    dst.open(QBuffer::WriteOnly);
    QDataStream ds(&dst);
    ds.setByteOrder(QDataStream::LittleEndian);
    // write root directory
    SubdirStorage subdirs;
    writeDirectory(dst, ds, m_root, subdirs, symTbl);
    // write subdirectories until we can't
    SubdirStorage nextSubdirs;
    QExeRsrcEntryPtr entry;
    while (!subdirs.empty()) {
        foreach (entry, subdirs.name)
            writeDirectory(dst, ds, entry, nextSubdirs, symTbl);
        foreach (entry, subdirs.id)
            writeDirectory(dst, ds, entry, nextSubdirs, symTbl);
        subdirs.move(nextSubdirs);
    }
    // write symbols
    writeSymbols(dst, ds, sizes, symTbl);
    dst.close();

    return sec;
}

bool QExeRsrcManager::toSection(QExe &exeDat)
{
    QExeSectionPtr sec = toSection(exeDat.optionalHeader()->sectionAlign);
    if (!exeDat.sectionManager()->addSection(sec))
        return false;
    correctOffsets(sec, sec->virtualAddr);
    return true;
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

bool QExeRsrcManager::readEntry(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr entry, quint32 offset)
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
        dataOff &= ~hiMask;
        child->m_type = QExeRsrcEntry::Directory;
        src.seek(dataOff);
        if (!readDirectory(src, ds, child, offset))
            return false;
    }
    src.seek(prevPos);
    return entry->addChild(child);
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

void QExeRsrcManager::writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, QExeRsrcManager::SubdirStorage &subdirs, QExeRsrcManager::SymbolTable &symTbl)
{
    symTbl.directories.push_front(dir);
    symTbl.directoryOffs[dir] = static_cast<quint32>(dst.pos());
    // write unimportant stuff
    ds << dir->directoryMeta.characteristics;
    ds << dir->directoryMeta.timestamp;
    ds << dir->directoryMeta.version;
    // count entries, save subdirectories for later
    quint16 entryCountName = 0, entryCountID = 0;
    std::list<QExeRsrcEntryPtr> entriesID, entriesName;
    QExeRsrcEntryPtr entry;
    foreach (entry, dir->children()) {
        if (entry->name.isEmpty()) {
            entryCountID++;
            entriesID.push_front(entry);
        } else {
            entryCountName++;
            entriesName.push_front(entry);
        }
        if (entry->type() == QExeRsrcEntry::Directory)
            subdirs.add(entry);
    }
    // write entry counts
    ds << entryCountName;
    ds << entryCountID;
    // write entries
    writeEntries(dst, ds, entriesName, symTbl);
    writeEntries(dst, ds, entriesID, symTbl);
}

void QExeRsrcManager::writeEntries(QBuffer &dst, QDataStream &ds, std::list<QExeRsrcEntryPtr> &entries, QExeRsrcManager::SymbolTable &symTbl)
{
    QExeRsrcEntryPtr entry;
    foreach (entry, entries) {
        if (entry->name.isEmpty())
            ds << entry->id;
        else {
            symTbl.strings.push_front(entry->name);
            symTbl.stringRefs[entry->name] += dst.pos();
            ds << hiMask;
        }
        if (entry->type() == QExeRsrcEntry::Directory) {
            symTbl.directoryRefs[entry] += dst.pos();
            ds << hiMask;
        } else {
            symTbl.dataDescs.push_front(entry);
            symTbl.dataDescRefs[entry] += dst.pos();
            ds << static_cast<quint32>(0);
            ds << static_cast<quint32>(entry->data.size());
            ds << entry->dataMeta.codepage;
            ds << entry->dataMeta.reserved;
        }
    }
}

void QExeRsrcManager::writeSymbols(QBuffer &dst, QDataStream &ds, const QExeRsrcManager::SectionSizes &sizes, QExeRsrcManager::SymbolTable &symTbl)
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
        dataPtrs[dataDesc] = static_cast<quint32>(dst.pos());
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
    }
}
