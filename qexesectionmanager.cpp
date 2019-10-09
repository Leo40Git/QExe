#include "qexesectionmanager.h"

#include <QBuffer>
#include <QtEndian>

#include "qexe.h"

inline quint32 QExeSectionManager::headerSize()
{
    return static_cast<quint32>(sections.size()) * 0x28;
}

QExeSectionManager::QExeSectionManager(QExe *exeDat, QObject *parent) : QObject(parent)
{
    m_exeDat = exeDat;
}

void QExeSectionManager::read(QIODevice &src)
{
    sections.clear();

    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    qint64 prev = 0;

    quint32 sectionCount = m_exeDat->coffHeader()->sectionCount;
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
        quint32 rawDataPtr = qFromLittleEndian<quint32>(buf32.data());
        newSec->linearize = newSec->virtualAddr == rawDataPtr;
        prev = src.pos();
        src.seek(rawDataPtr);
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
    quint32 fileAlign = m_exeDat->optionalHeader()->fileAlign;
    // set out section data in physical space
    quint32 currentPos = QExe::alignForward(static_cast<quint32>(dst.pos()) + headerSize(), fileAlign);
    QVector<quint32> rawDataPtrs;
    QSharedPointer<QExeSection> section;
    foreach (section, sections) {
        if (section->linearize) {
            if (currentPos < section->virtualAddr)
                currentPos = section->virtualAddr;
        }
        rawDataPtrs += currentPos;
        currentPos += static_cast<quint32>(section->rawData.size());
        currentPos = QExe::alignForward(currentPos, fileAlign);
    }
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
        qToLittleEndian<quint32>(rawDataPtrs[i], buf32.data());
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
        dst.seek(rawDataPtrs[i]);
        dst.write(section->rawData);
    }
}

// TODO
bool QExeSectionManager::test(QExeErrorInfo *errinfo)
{
    (void)errinfo;
    return true;
}
