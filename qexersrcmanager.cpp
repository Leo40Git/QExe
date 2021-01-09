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

void QExeRsrcManager::shiftOffsets(QExeSectionPtr rsrcSec, const qint64 shift)
{
    QBuffer buf(rsrcSec.data());
    buf.open(QBuffer::ReadWrite);
    QDataStream ds(&buf);
    shiftDirectory(buf, ds, shift);
    buf.close();
}

void QExeRsrcManager::shiftDirectory(QBuffer &buf, QDataStream &ds, const qint64 shift)
{
    quint16 entriesID, entriesName;
    buf.seek(buf.pos() + 12);
    ds >> entriesName;
    ds >> entriesID;
    quint16 entries = entriesID + entriesName;
    for (quint16 i = 0; i < entries; i++) {
        quint32 rva;
        ds >> rva;
        qint64 pos = buf.pos();
        if ((rva & hiMask) != 0) {
            buf.seek(rva & ~hiMask);
            shiftDirectory(buf, ds, shift);
        } else {
            buf.seek(rva);
            quint32 val;
            ds >> val;
            buf.seek(rva);
            ds << static_cast<quint32>(val + shift);
        }
        buf.seek(pos);
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
            shiftOffsets(rsrcSec, rsrcSec->virtualAddr - oldRVA);
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

inline QString entryID(QExeRsrcEntryPtr e) {
    if (e->name.isEmpty())
        return QString("0x%1").arg(QString::number(e->id, 16).toUpper());
    else
        return e->name;
}

#include <QDebug>
#define OUT qInfo().noquote().nospace()
#define HEX(n) "0x" << QString::number(n, 16).toUpper()

QExeSectionPtr QExeRsrcManager::toSection(quint32 sectionAlign)
{
    SymbolTable symTbl;
    SectionSizes sizes = calculateSectionSizes(m_root);
    quint32 size = QExe::alignForward(sizes.totalSize(), sectionAlign);
    QExeSectionPtr sec = QExeSectionPtr(new QExeSection(QLatin1String(".rsrc"), size,
                                                        QExeSection::ContainsInitializedData | QExeSection::IsReadable));

    QBuffer buf(&sec->rawData);
    buf.open(QBuffer::WriteOnly);
    QDataStream ds(&buf);
    ds.setByteOrder(QDataStream::LittleEndian);
    // TODO
    buf.close();

    return sec;
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
