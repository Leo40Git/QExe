#include "qexesectionmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QDataStream>

#include <algorithm>

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

bool QExeSectionManager::containsSection(QExeSectionPtr sec) const
{
    return sections.contains(sec);
}

QVector<bool> QExeSectionManager::containsSections(QVector<QExeSectionPtr> secs) const
{
    QVector<bool> ret;
    QExeSectionPtr sec;
    foreach (sec, secs)
        ret += containsSection(sec);
    return ret;
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

QVector<bool> QExeSectionManager::addSections(QVector<QExeSectionPtr> newSecs)
{
    QVector<bool> ret;
    QExeSectionPtr sec;
    foreach (sec, newSecs)
        ret += addSection(sec);
    return ret;
}

bool QExeSectionManager::removeSection(QExeSectionPtr sec)
{
    return sections.removeOne(sec);
}

QVector<bool> QExeSectionManager::removeSections(QVector<QExeSectionPtr> secs)
{
    QVector<bool> ret;
    QExeSectionPtr sec;
    foreach (sec, secs)
        ret += removeSection(sec);
    return ret;
}

QExeSectionPtr QExeSectionManager::removeSection(int index)
{
    if (index < 0 || index >= sections.size())
        return nullptr;
    QExeSectionPtr sec = sections[index];
    sections.removeAt(index);
    return sec;
}

QVector<QExeSectionPtr> QExeSectionManager::removeSections(QVector<int> indexes)
{
    QVector<QExeSectionPtr> ret;
    for (int i = 0; i < indexes.size(); i++) {
        int index = indexes[i];
        if (index < 0 || index >= sections.size()) {
            ret[i] = nullptr;
            continue;
        }
        ret[i] = sections[i];
        sections[i] = nullptr;
    }
    sections.removeAll(nullptr);
    return ret;
}

QExeSectionPtr QExeSectionManager::removeSection(const QLatin1String &name)
{
    return removeSection(sectionIndexByName(name));
}

QVector<QExeSectionPtr> QExeSectionManager::removeSections(QVector<QLatin1String> names)
{
    QVector<QExeSectionPtr> ret;
    QLatin1String name;
    foreach (name, names)
        ret += removeSection(name);
    return ret;
}

QExeSectionPtr QExeSectionManager::createSection(const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars)
{
    return createSectionInternal(QExeSectionPtr(new QExeSection(name, data, chars)));
}

QExeSectionPtr QExeSectionManager::createSection(const QLatin1String &name, quint32 size, QExeSection::Characteristics chars)
{
    return createSectionInternal(QExeSectionPtr(new QExeSection(name, size, chars)));
}

QExeSectionPtr QExeSectionManager::createSectionInternal(QExeSectionPtr newSec)
{
    int secs;
    if ((secs = sectionCount()) > 0) {
        QExeSectionPtr lastSec = sections[secs - 1];
        newSec->virtualAddr = lastSec->virtualAddr;
        newSec->virtualAddr += QExe::alignForward(lastSec->virtualSize, exeDat->optionalHeader()->sectionAlign);
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

bool QExeSectionManager::read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo)
{
    (void)errinfo;
    sections.clear();

    quint32 sectionCount = exeDat->coffHeader()->sectionCount;
    quint32 rawDataSize;
    qint64 prev = 0;
    for (quint32 i = 0; i < sectionCount; i++) {
        QExeSectionPtr newSec = QExeSectionPtr(new QExeSection());
        newSec->nameBytes = src.read(8);
        ds >> newSec->virtualSize;
        ds >> newSec->virtualAddr;
        ds >> rawDataSize;
        ds >> newSec->rawDataPtr;
        newSec->linearize = newSec->virtualAddr == newSec->rawDataPtr;
        prev = src.pos();
        src.seek(newSec->rawDataPtr);
        newSec->rawData = src.read(rawDataSize);
        src.seek(prev);
        ds >> newSec->relocsPtr;
        ds >> newSec->linenumsPtr;
        ds >> newSec->relocsCount;
        ds >> newSec->linenumsCount;
        quint32 charsRaw;
        ds >> charsRaw;
        newSec->characteristics = static_cast<QExeSection::Characteristics>(charsRaw);
        sections += newSec;
    }
    return test(true, nullptr, errinfo);
}

bool QExeSectionManager::write(QIODevice &dst, QDataStream &ds, QExeErrorInfo *errinfo)
{
    (void)errinfo;
    QExeSectionPtr section;
    // write section headers
    foreach (section, sections) {
        section->nameBytes.resize(8);
        dst.write(section->nameBytes);
        ds << section->virtualSize;
        ds << section->virtualAddr;
        ds << static_cast<quint32>(section->rawData.size());
        ds << section->rawDataPtr;
        ds << section->relocsPtr;
        ds << section->linenumsPtr;
        ds << section->relocsCount;
        ds << section->linenumsCount;
        ds << static_cast<quint32>(section->characteristics);
    }
    // write section data
    foreach (section, sections) {
        if (section->rawData.size() == 0)
            continue;
        dst.seek(section->rawDataPtr);
        dst.write(section->rawData);
    }
    return true;
}

struct AllocSpan {
public:
    quint32 start;
    quint32 length;
    AllocSpan(quint32 start, quint32 length) {
        this->start = start;
        this->length = length;
    }
    bool within(quint32 target) const {
        return (target >= start) && ((start + length) > target);
    }
    bool collides(const AllocSpan &other) const {
        return within(other.start)
                || within(other.start + other.length - 1)
                || other.within(start)
                || other.within(start + length - 1);
    }
};

typedef std::list<AllocSpan> AllocMap;

bool checkAlloc(AllocMap &map, AllocSpan span, bool add = true) {
    auto result = std::find_if(map.begin(), map.end(), [&span](const AllocSpan &chk) {
        return chk.collides(span);
    });
    if (result != map.end())
        return false;
    if (add)
        map.push_back(span);
    return true;
}

bool QExeSectionManager::test(bool justOrderAndOverlap, quint32 *fileSize, QExeErrorInfo *errinfo)
{
    QExeSectionPtr section;
    // -- Test virtual integrity
    std::sort(sections.begin(), sections.end(), [](const QExeSectionPtr &s1, const QExeSectionPtr &s2) {
        return s1->virtualAddr < s2->virtualAddr;
    });
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
    map.push_back(AllocSpan(0, exeDat->optionalHeader()->headerSize));
    for (int i = 0; i < 2; i++) {
        foreach (section, sections) {
            if (section->linearize != (i == 0))
                continue;
            quint32 rawDataSize = static_cast<quint32>(section->rawData.size());
            if (rawDataSize == 0) {
                // don't bother with this section
                section->rawDataPtr = 0;
                continue;
            }
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
        return true;
    // -- Calculate file size
    *fileSize = 0;
    AllocSpan span = AllocSpan(0, 0);
    foreach (span, map) {
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
    map.push_back(AllocSpan(0, exeDat->optionalHeader()->headerSize));
    // add other sections
    QExeSectionPtr section;
    foreach (section, sections) {
        map.push_back(AllocSpan(section->virtualAddr, QExe::alignForward(section->virtualSize, sectionAlign)));
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
