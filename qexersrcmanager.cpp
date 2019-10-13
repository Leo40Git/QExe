#include "qexersrcmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QTextCodec>

#include "qexe.h"

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }

constexpr quint32 hiMask = 0x80000000;

#include <QDebug>
#define OUT qDebug().noquote().nospace()
#define HEX(n) "0x" << QString::number(n, 16).toUpper()

quint32 QExeRsrcManager::headerSize() const
{
    // our "header" is actually the .rsrc section's header, and section headers are always 0x28 bytes
    return 0x28;
}

QExeRsrcEntryPtr QExeRsrcManager::root() const
{
    return m_root;
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
    m_root->removeAllChildren();
    OUT << "Reading root directory";
    if (!readDirectory(buf, m_root, sec->virtualAddr)) {
        SET_ERROR_INFO(BadRsrc_InvalidFormat)
        return false;
    }
    buf.close();
    return true;
}

class QExeRsrcManager::SectionSizes {
public:
    quint32 directorySize;
    quint32 dataEntrySize;
    quint32 stringSize;
    quint32 dataSize;
    quint32 totalSize() {
        return directorySize + dataEntrySize + stringSize + dataSize;
    }
    SectionSizes& operator+=(const SectionSizes &other) {
        directorySize += other.directorySize;
        dataEntrySize += other.dataEntrySize;
        stringSize += other.stringSize;
        dataSize += other.dataSize;
        return *this;
    }
};

class QExeRsrcManager::ReferenceMemory {
public:
    QMap<QExeRsrcEntryPtr, quint32> directoryOffs;
    QMap<QExeRsrcEntryPtr, QList<qint64>> directoryRefs;
    QMap<QExeRsrcEntryPtr, QList<qint64>> dataEntryRefs;
    QMap<QString, QList<qint64>> stringRefs;
};

void QExeRsrcManager::toSection()
{
    ReferenceMemory refMem;
    SectionSizes sizes = calculateSectionSizes(m_root);
    QExeSectionPtr sec = exeDat->sectionManager()->createSection(QLatin1String(".rsrc"), sizes.totalSize());
    OUT << "sec->virtualAddr = " << HEX(sec->virtualAddr);
    sec->characteristics = QExeSection::ContainsInitializedData | QExeSection::IsReadable;
    QBuffer buf(&sec->rawData);
    buf.open(QBuffer::WriteOnly);
    writeDirectory(buf, m_root, refMem);
    writeReferences(buf, sizes, refMem, sec->virtualAddr);
    buf.close();
}

bool QExeRsrcManager::readDirectory(QBuffer &src, QExeRsrcEntryPtr dir, quint32 offset) {
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    src.read(buf32.data(), sizeof(quint32));
    dir->directoryMeta.characteristics = qFromLittleEndian<quint32>(buf32.data());
    OUT << " characteristics = " << HEX(dir->directoryMeta.characteristics);
    src.read(buf32.data(), sizeof(quint32));
    dir->directoryMeta.timestamp = qFromLittleEndian<quint32>(buf32.data());
    OUT << " timestamp = " << HEX(dir->directoryMeta.timestamp);
    src.read(buf16.data(), sizeof(quint16));
    dir->directoryMeta.version.first = qFromLittleEndian<quint16>(buf16.data());
    src.read(buf16.data(), sizeof(quint16));
    dir->directoryMeta.version.second = qFromLittleEndian<quint16>(buf16.data());
    OUT << " version = " << dir->directoryMeta.version.first << "." << dir->directoryMeta.version.second;
    src.read(buf16.data(), sizeof(quint16));
    quint16 entriesName = qFromLittleEndian<quint16>(buf16.data());
    src.read(buf16.data(), sizeof(quint16));
    quint16 entriesID = qFromLittleEndian<quint16>(buf16.data());
    quint16 entries = entriesID + entriesName;
    OUT << " " << entriesName << " entries with name, " << entriesID << " entries with ID (" << entries << " entries total)";
    for (quint32 i = 0; i < entries; i++)
        if (!readEntry(src, dir, offset)) {
            OUT << " READ FAIL - readEntry returned false";
            return false;
        }
    return true;
}

bool QExeRsrcManager::readEntry(QBuffer &src, QExeRsrcEntryPtr dir, quint32 offset)
{
    OUT << " reading entry";
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(QExeRsrcEntry::Data));

    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    // read name/ID
    src.read(buf32.data(), sizeof(quint32));
    quint32 nameOff = qFromLittleEndian<quint32>(buf32.data());
    qint64 prevPos = src.pos();
    if ((nameOff & hiMask) == 0) {
        // id
        child->id = nameOff;
        OUT << "  id = " << HEX(nameOff);
    } else {
        // name
        nameOff &= ~hiMask;
        src.seek(nameOff);
        src.read(buf16.data(), sizeof(quint16));
        quint16 nameLen = qFromLittleEndian<quint16>(buf16.data());
        QByteArray nameBytes = src.read(nameLen * 2);
        child->name = QTextCodec::codecForName("UTF-16LE")->makeDecoder(QTextCodec::IgnoreHeader)->toUnicode(nameBytes);
        src.seek(prevPos);
        OUT << "  name = " << child->name;
    }
    // read data/directory
    src.read(buf32.data(), sizeof(quint32));
    quint32 dataOff = qFromLittleEndian<quint32>(buf32.data());
    prevPos = src.pos();
    if ((dataOff & hiMask) == 0) {
        // data
        OUT << "  it's data!";
        src.seek(dataOff);
        src.read(buf32.data(), sizeof(quint32));
        quint32 dataPtr = qFromLittleEndian<quint32>(buf32.data()) - offset;
        OUT << "   ptr = " << HEX(dataPtr);
        src.read(buf32.data(), sizeof(quint32));
        quint32 dataSize = qFromLittleEndian<quint32>(buf32.data());
        OUT << "   size = " << HEX(dataSize);
        src.read(buf32.data(), sizeof(quint32));
        child->dataMeta.codepage = qFromLittleEndian<quint32>(buf32.data());
        OUT << "   codepage = " << HEX(child->dataMeta.codepage);
        src.read(buf32.data(), sizeof(quint32));
        child->dataMeta.reserved = qFromLittleEndian<quint32>(buf32.data());
        OUT << "   reserved = " << HEX(child->dataMeta.reserved);
        src.seek(dataPtr);
        child->data = src.read(dataSize);
    } else {
        // directory
        OUT << "  it's a subdirectory!";
        dataOff &= ~hiMask;
        child->m_type = QExeRsrcEntry::Directory;
        src.seek(dataOff);
        if (!readDirectory(src, child, offset)) {
            OUT << "READ FAIL - readDirectory returned false";
            return false;
        }
    }
    src.seek(prevPos);
    if (dir->addChild(child))
        return true;
    else {
        OUT << "READ FAIL - dir->addChild returned false";
        return false;
    }
}

QExeRsrcManager::SectionSizes QExeRsrcManager::calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr)
{
    if (allocStr == nullptr)
        allocStr = new QStringList();
    SectionSizes sz;
    sz.directorySize = 0x10;
    sz.dataEntrySize = 0;
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
            sz.dataEntrySize += 0x10;
            sz.dataSize += static_cast<quint32>(entry->data.size());
        } else {
            SectionSizes other = calculateSectionSizes(entry, allocStr);
            sz += other;
        }
    }
    return sz;
}

void QExeRsrcManager::writeDirectory(QBuffer &dst, QExeRsrcEntryPtr dir, QExeRsrcManager::ReferenceMemory &refMem)
{
    refMem.directoryOffs[dir] = static_cast<quint32>(dst.pos());
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    // write unimportant fields
    qToLittleEndian<quint32>(dir->directoryMeta.characteristics, buf32.data());
    dst.write(buf32);
    qToLittleEndian<quint32>(dir->directoryMeta.timestamp, buf32.data());
    dst.write(buf32);
    qToLittleEndian<quint16>(dir->directoryMeta.version.first, buf16.data());
    dst.write(buf16);
    qToLittleEndian<quint16>(dir->directoryMeta.version.second, buf16.data());
    dst.write(buf16);
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
    qToLittleEndian<quint16>(entryCountName, buf16.data());
    dst.write(buf16);
    qToLittleEndian<quint16>(entryCountID, buf16.data());
    dst.write(buf16);
    // second romp to actually write entries
    // make a subdir list to write *after* writing directory
    QLinkedList<QExeRsrcEntryPtr> subdirs;
    writeEntries(dst, entriesName, subdirs, refMem);
    writeEntries(dst, entriesID, subdirs, refMem);
    // now write subdirs
    foreach (entry, subdirs)
        writeDirectory(dst, entry, refMem);
}

void QExeRsrcManager::writeEntries(QBuffer &dst, QLinkedList<QExeRsrcEntryPtr> entries, QLinkedList<QExeRsrcEntryPtr> &subdirs, QExeRsrcManager::ReferenceMemory &refMem)
{
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    QExeRsrcEntryPtr entry;
    foreach (entry, entries) {
        if (entry->name.isEmpty()) {
            qToLittleEndian<quint32>(entry->id, buf32.data());
            dst.write(buf32);
        } else {
            refMem.stringRefs[entry->name] += dst.pos();
            qToLittleEndian<quint32>(~hiMask, buf32.data());
            dst.write(buf32);
        }
        if (entry->type() == QExeRsrcEntry::Data) {
            refMem.dataEntryRefs[entry] += dst.pos();
            qToLittleEndian<quint32>(0, buf32.data());
            dst.write(buf32);
        } else {
            refMem.directoryRefs[entry] += dst.pos();
            subdirs += entry;
            qToLittleEndian<quint32>(~hiMask, buf32.data());
            dst.write(buf32);
        }
    }
}

void QExeRsrcManager::writeReferences(QBuffer &dst, QExeRsrcManager::SectionSizes sizes, QExeRsrcManager::ReferenceMemory &refMem, quint32 offset)
{
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    // write directroy references
    QMapIterator<QExeRsrcEntryPtr, QList<qint64>> dirRefsI(refMem.directoryRefs);
    while (dirRefsI.hasNext()) {
        dirRefsI.next();
        qToLittleEndian<quint32>(refMem.directoryOffs[dirRefsI.key()] | hiMask, buf32.data());
        qint64 pos;
        foreach (pos, dirRefsI.value()) {
            dst.seek(pos);
            dst.write(buf32);
        }
    }
    // write actual data, remember offsets
    QMap<QExeRsrcEntryPtr, quint32> dataPtrs;
    dst.seek(sizes.directorySize + sizes.dataEntrySize + sizes.stringSize);
    QMapIterator<QExeRsrcEntryPtr, QList<qint64>> dataEntryRefsI(refMem.dataEntryRefs);
    while (dataEntryRefsI.hasNext()) {
        dataEntryRefsI.next();
        dataPtrs[dataEntryRefsI.key()] = static_cast<quint32>(dst.pos()) + offset;
        dst.write(dataEntryRefsI.key()->data);
    }
    // write data entries and their references
    dst.seek(sizes.directorySize);
    dataEntryRefsI.toFront();
    while (dataEntryRefsI.hasNext()) {
        dataEntryRefsI.next();
        quint32 prevPos = static_cast<quint32>(dst.pos());
        qToLittleEndian<quint32>(prevPos, buf32.data());
        qint64 pos;
        foreach (pos, dataEntryRefsI.value()) {
            dst.seek(pos);
            dst.write(buf32);
        }
        dst.seek(prevPos);
        QExeRsrcEntryPtr entry = dataEntryRefsI.key();
        quint32 dataPtr = dataPtrs[entry];
        qToLittleEndian<quint32>(dataPtr, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(static_cast<quint32>(entry->data.size()), buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(entry->dataMeta.codepage, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(entry->dataMeta.reserved, buf32.data());
        dst.write(buf32);
    }
    // write strings and their references
    QTextEncoder *encoder = QTextCodec::codecForName("UTF-16LE")->makeEncoder(QTextCodec::IgnoreHeader);
    dst.seek(sizes.directorySize + sizes.dataEntrySize);
    QMapIterator<QString, QList<qint64>> stringRefsI(refMem.stringRefs);
    while (stringRefsI.hasNext()) {
        stringRefsI.next();
        quint32 prevPos = static_cast<quint32>(dst.pos());
        qToLittleEndian<quint32>(prevPos | hiMask, buf32.data());
        qint64 pos;
        foreach (pos, stringRefsI.value()) {
            dst.seek(pos);
            dst.write(buf32);
        }
        dst.seek(prevPos);
        QString str = stringRefsI.key();
        qToLittleEndian<quint16>(static_cast<quint16>(str.size()), buf16.data());
        dst.write(buf16);
        dst.write(encoder->fromUnicode(str));
    }
}
