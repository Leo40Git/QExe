#include "qexesectionmanager.h"

#include <QBuffer>
#include <QtEndian>
#include <QLinkedList>

#include "qexe.h"

quint32 QExeSectionManager::headerSize()
{
    return static_cast<quint32>(sections.size()) * 0x28;
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
        QSharedPointer<QExeSection> newSec = QSharedPointer<QExeSection>(new QExeSection());
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
    QSharedPointer<QExeSection> section;
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
    bool within(quint32 target) {
        return target >= start && (start + length) > target;
    }
    bool collides(AllocSpan *other) {
        return within(other->start) || within(other->start + other->length - 1) || other->within(start) || other->within(start + length - 1);
    }
};

typedef QLinkedList<AllocSpan*> AllocMap;

bool checkAlloc(AllocMap &map, AllocSpan *span) {
    AllocSpan *chk;
    foreach (chk, map) {
        if (chk->collides(span))
            return false;
    }
    map += span;
    return true;
}

void deleteAllocs(AllocMap &map) {
    AllocSpan *span;
    foreach (span, map) {
        delete span;
    }
    map.clear();
}

bool QExeSectionManager::test(QExeErrorInfo *errinfo)
{
    QSharedPointer<QExeSection> section;
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
    // -- Allocate file addresses
    quint32 fileAlign = exeDat->optionalHeader()->fileAlign;
    AllocMap map;
    // disallow collision with primary header
    map += new AllocSpan(0, exeDat->optionalHeader()->headerSize);
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
                    deleteAllocs(map);
                    return false;
                }
                ok = checkAlloc(map, new AllocSpan(section->virtualAddr, rawDataSize));
                section->rawDataPtr = section->virtualAddr;
            }
            if (!ok) {
                quint32 pos = 0;
                while (!checkAlloc(map, new AllocSpan(pos, rawDataSize)))
                    pos += fileAlign;
                section->rawDataPtr = pos;
            }
        }
    }
    deleteAllocs(map);
    return true;
}
