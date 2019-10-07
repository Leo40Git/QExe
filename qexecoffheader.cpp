#include "qexecoffheader.h"

#include <QBuffer>
#include <QtEndian>

using namespace ErrorInfo;
#define SET_ERROR_INFO(errName, subName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = errName; \
        errinfo->subID = errName ## _ ## subName; \
    }

QExeCOFFHeader::QExeCOFFHeader(QObject *parent) : QObject(parent)
{
    machineType = I386;
}

bool QExeCOFFHeader::read(QByteArray src, ErrorInfo::ErrorInfoStruct *errinfo)
{
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    QBuffer buf(&src);
    buf.open(QBuffer::ReadOnly);
    buf.read(buf16.data(), sizeof(quint16));
    machineType = static_cast<MachineType>(qFromLittleEndian<quint16>(buf16.data()));
    buf.read(buf16.data(), sizeof(quint16));
    sectionCount = qFromLittleEndian<quint16>(buf16.data());
    // "Note that the Windows loader limits the number of sections to 96." (https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image)
    if (sectionCount > 96) {
        SET_ERROR_INFO(BadPEFile, InvalidSectionCount)
        errinfo->details += sectionCount;
        return false;
    }
    buf.read(buf32.data(), sizeof(quint32));
    timestamp = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    symTblPtr = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    symTblCount = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf16.data(), sizeof(quint16));
    optHeadSize = qFromLittleEndian<quint16>(buf16.data());
    buf.read(buf16.data(), sizeof(quint16));
    characteristics = Characteristics(qFromLittleEndian<quint16>(buf16.data()));
    buf.close();
    return true;
}

QByteArray QExeCOFFHeader::toBytes()
{
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    QByteArray out(0x14, 0);
    QBuffer buf(&out);
    buf.open(QBuffer::WriteOnly);
    qToLittleEndian<quint16>(machineType, buf16.data());
    buf.write(buf16);
    qToLittleEndian<quint16>(sectionCount, buf16.data());
    buf.write(buf16);
    qToLittleEndian<quint32>(timestamp, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(symTblPtr, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(symTblCount, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint16>(optHeadSize, buf16.data());
    buf.write(buf16);
    qToLittleEndian<quint16>(static_cast<quint16>(characteristics), buf16.data());
    buf.write(buf16);
    buf.close();
    return out;
}
