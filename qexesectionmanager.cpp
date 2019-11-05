#include "qexesectionmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QLinkedList>

#include "qexe.h"

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }

quint32 QExeSectionManager::headerSize() const
{
    return static_cast<quint32>(sections.size()) * 0x28;
}

int QExeSectionManager::sectionCount() const
{
    return sections.size();
}

QExeSectionPtr QExeSectionManager::sectionAt(int index) const
{
    if (index < 0 || index >= sections.size())
        return nullptr;
    return sections[index];
}

int QExeSectionManager::sectionIndexByName(const QLatin1String &name) const
{
    for (int i = 0; i < sections.size(); i++) {
        if (sections[i]->name() == name)
            return i;
    }
    return -1;
}

QExeSectionPtr QExeSectionManager::sectionWithName(const QLatin1String &name) const
{
    return sectionAt(sectionIndexByName(name));
}

bool QExeSectionManager::addSection(QExeSectionPtr newSec)
{
    if (newSec.isNull())
        return false;
    QLatin1String newSecName = newSec->name();
    QExeSectionPtr section;
    // having 2 sections with the same name isn't explicitly disallowed by the PE format docs,
    // but most "sane" linkers don't do that, so we can safely disallow that
    foreach (section, sections) {
        if (newSecName == section->name())
            return false;
    }
    quint32 headerSize = exeDat->optionalHeader()->headerSize;
    if (newSec->virtualAddr < headerSize)
        newSec->virtualAddr = headerSize;
    positionSection(newSec, newSec->virtualAddr, exeDat->optionalHeader()->sectionAlign);
    sections += newSec;
    exeDat->updateHeaderSizes();
    return true;
}

QExeSectionPtr QExeSectionManager::removeSection(int index)
{
    if (index < 0 || index >= sections.size())
        return nullptr;
    QExeSectionPtr sec = sections[index];
    sections.removeAt(index);
    return sec;
}

QExeSectionPtr QExeSectionManager::removeSection(const QLatin1String &name)
{
    return removeSection(sectionIndexByName(name));
}

QExeSectionPtr QExeSectionManager::createSection(const QLatin1String &name, quint32 size)
{
    QExeSectionPtr newSec = QExeSectionPtr(new QExeSection(name, size));
    newSec->linearize = false;
    int secs;
    if ((secs = sectionCount()) > 0) {
        QExeSectionPtr lastSec = sections[secs - 1];
        newSec->virtualAddr = QExe::alignForward(lastSec->virtualAddr + lastSec->virtualSize, exeDat->optionalHeader()->sectionAlign);
    }
    if (!addSection(newSec))
        return nullptr;
    return newSec;
}

QBuffer *QExeSectionManager::setupRVAPoint(quint32 rva, QIODevice::OpenMode mode) const
{
    rva %= exeDat->optionalHeader()->imageBase;
    QExeSectionPtr section;
    foreach (section, sections) {
        if (rva >= section->virtualAddr) {
            quint32 rel = rva - section->virtualAddr;
            if (rel < qMax(static_cast<quint32>(section->rawData.size()), section->virtualSize)) {
                QBuffer *out = new QBuffer(&section->rawData);
                out->open(mode);
                out->seek(rel);
                return out;
            }
        }
    }
    return nullptr;
}

int QExeSectionManager::rsrcSectionIndex()
{
    QList<DataDirectoryPtr> dataDirs = exeDat->optionalHeader()->dataDirectories;
    if (dataDirs.size() <= QExeOptionalHeader::ResourceTable)
        return -1;
    quint32 rsrcVA = dataDirs[QExeOptionalHeader::ResourceTable]->first;
    for (int i = 0; i < sections.size(); i++)
        if (sections[i]->virtualAddr == rsrcVA)
            return i;
    return -1;
}

QExeSectionManager::QExeSectionManager(QExe *exeDat, QObject *parent) : QObject(parent)
{
    this->exeDat = exeDat;
}

void QExeSectionManager::read(QIODevice &src)
{
    sections.clear();

    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    qint64 prev = 0;

    quint32 sectionCount = exeDat->coffHeader()->sectionCount;
    for (quint32 i = 0; i < sectionCount; i++) {
        QExeSectionPtr newSec = QExeSectionPtr(new QExeSection());
        newSec->nameBytes = src.read(8);
        src.read(buf32.data(), sizeof(quint32));
        newSec->virtualSize = qFromLittleEndian<quint32>(buf32.data());
        src.read(buf32.data(), sizeof(quint32));
        newSec->virtualAddr = qFromLittleEndian<quint32>(buf32.data());
        src.read(buf32.data(), sizeof(quint32));
        quint32 rawDataSize = qFromLittleEndian<quint32>(buf32.data());
        src.read(buf32.data(), sizeof(quint32));
        newSec->rawDataPtr = qFromLittleEndian<quint32>(buf32.data());
        newSec->linearize = newSec->virtualAddr == newSec->rawDataPtr;
        prev = src.pos();
        src.seek(newSec->rawDataPtr);
        newSec->rawData = src.read(rawDataSize);
        src.seek(prev);
        src.read(buf32.data(), sizeof(quint32));
        newSec->relocsPtr = qFromLittleEndian<quint32>(buf32.data());
        src.read(buf32.data(), sizeof(quint32));
        newSec->linenumsPtr = qFromLittleEndian<quint32>(buf32.data());
        src.read(buf16.data(), sizeof(quint16));
        newSec->relocsCount = qFromLittleEndian<quint16>(buf16.data());
        src.read(buf16.data(), sizeof(quint16));
        newSec->linenumsCount = qFromLittleEndian<quint16>(buf32.data());
        src.read(buf32.data(), sizeof(quint32));
        newSec->characteristics = QExeSection::Characteristics(qFromLittleEndian<quint32>(buf32.data()));
        sections += newSec;
    }
}

void QExeSectionManager::write(QIODevice &dst)
{
    QExeSectionPtr section;
    // write section headers
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    for (int i = 0; i < sections.size(); i++) {
        section = sections[i];
        dst.write(section->nameBytes);
        qToLittleEndian<quint32>(section->virtualSize, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(section->virtualAddr, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(static_cast<quint32>(section->rawData.size()), buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(section->rawDataPtr, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(section->relocsPtr, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint32>(section->linenumsPtr, buf32.data());
        dst.write(buf32);
        qToLittleEndian<quint16>(section->relocsCount, buf16.data());
        dst.write(buf16);
        qToLittleEndian<quint16>(section->linenumsCount, buf16.data());
        dst.write(buf16);
        qToLittleEndian<quint32>(section->characteristics, buf32.data());
        dst.write(buf32);
    }
    // write section data
    for (int i = 0; i < sections.size(); i++) {
        section = sections[i];
        dst.seek(section->rawDataPtr);
        dst.write(section->rawData);
    }
}

bool sectionVALessThan(const QSharedPointer<QExeSection> &s1, const QSharedPointer<QExeSection> &s2) {
    return s1->virtualAddr < s2->virtualAddr;
}

class AllocSpan {
public:
    quint32 start;
    quint32 length;
    AllocSpan(quint32 start, quint32 length) {
        this->start = start;
        this->length = length;
    }
    bool within(quint32 target) const {
        return target >= start && (start + length) > target;
    }
    bool collides(const AllocSpan &other) const {
        return within(other.start) || within(other.start + other.length - 1) || other.within(start) || other.within(start + length - 1);
    }
};

typedef QLinkedList<AllocSpan> AllocMap;

bool checkAlloc(AllocMap &map, AllocSpan span, bool add = true) {
    QLinkedListIterator<AllocSpan> mapI(map);
    while (mapI.hasNext()) {
        const AllocSpan &chk = mapI.next();
        if (chk.collides(span))
            return false;
    }
    if (add)
        map += span;
    return true;
}

bool QExeSectionManager::test(bool justOrderAndOverlap, quint32 *fileSize, QExeErrorInfo *errinfo)
{
    QExeSectionPtr section;
    // -- Test virtual integrity
    std::sort(sections.begin(), sections.end(), sectionVALessThan);
    // set minimum usable RVA
    quint32 rvaHeaderFloor = 0;
    foreach (section, sections) {
        if (section->virtualAddr < rvaHeaderFloor) {
            if (errinfo != nullptr) {
                errinfo->errorID = QExeErrorInfo::BadSection_VirtualOverlap;
                errinfo->details += section->name();
            }
            return false;
        }
        rvaHeaderFloor = section->virtualAddr + section->virtualSize;
    }
    if (justOrderAndOverlap)
        return true;
    // -- Allocate file addresses
    quint32 fileAlign = exeDat->optionalHeader()->fileAlign;
    AllocMap map;
    // disallow collision with primary header
    map += AllocSpan(0, exeDat->optionalHeader()->headerSize);
    for (int i = 0; i < 2; i++) {
        foreach (section, sections) {
            if (section->linearize != (i == 0))
                continue;
            quint32 rawDataSize = static_cast<quint32>(section->rawData.size());
            bool ok = false;
            if (section->linearize) {
                if (QExe::alignForward(section->virtualAddr, fileAlign) != section->virtualAddr) {
                    if (errinfo != nullptr) {
                        errinfo->errorID = QExeErrorInfo::BadSection_LinearizeFailure;
                        errinfo->details += section->name();
                    }
                    return false;
                }
                ok = checkAlloc(map, AllocSpan(section->virtualAddr, rawDataSize));
                section->rawDataPtr = section->virtualAddr;
            }
            if (!ok) {
                AllocSpan span(0, rawDataSize);
                while (!checkAlloc(map, span))
                    span.start += fileAlign;
                section->rawDataPtr = span.start;
            }
        }
    }
    if (!fileSize)
        return false;
    // -- Calculate file size
    *fileSize = 0;
    QLinkedListIterator<AllocSpan> mapI(map);
    while (mapI.hasNext()) {
        const AllocSpan &span = mapI.next();
        quint32 v = span.start + span.length;
        if (v > *fileSize)
            *fileSize = v;
    }
    QExe::alignForward(*fileSize, fileAlign);
    return true;
}

void QExeSectionManager::positionSection(QExeSectionPtr newSec, quint32 i, quint32 sectionAlign)
{
    AllocMap map;
    // disallow collision with primary header
    map += AllocSpan(0, exeDat->optionalHeader()->headerSize);
    // add other sections
    QExeSectionPtr section;
    foreach (section, sections) {
        map += AllocSpan(section->virtualAddr, QExe::alignForward(section->virtualSize, sectionAlign));
    }
    AllocSpan as(0, QExe::alignForward(newSec->virtualSize, sectionAlign));
    while (true) {
        i = QExe::alignForward(i, sectionAlign);
        // is this OK?
        as.start = i;
        if (checkAlloc(map, as, false)) {
            newSec->virtualAddr = i;
            break;

        }
        i += sectionAlign;
    }
}
