#include "qexe.h"

#include <QtEndian>
#include <QBuffer>

QExe::QExe(QObject *parent) : QObject(parent)
{
    reset();
}

void QExe::reset()
{
    dosStub = new QExeDOSStub(this);
    coffHead = new QExeCOFFHeader(this);
    optHead = new QExeOptHeader(this);
}

using namespace ErrorInfo;
#define SET_ERROR_INFO(errName, subName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = errName; \
        errinfo->subID = errName ## _ ## subName; \
    }

bool QExe::read(QIODevice &src, ErrorInfoStruct *errinfo)
{
    // make sure src is usable
    if (!src.isReadable()) {
        SET_ERROR_INFO(BadIODevice, Unreadable)
        return false;
    }
    if (src.isSequential()) {
        SET_ERROR_INFO(BadIODevice, Sequential)
        return false;
    }

    QByteArray buf32(sizeof(quint32), 0);
    // read DOS stub size
    src.seek(0x3C);
    src.read(buf32.data(), sizeof(quint32));
    qint64 dosSz = qFromLittleEndian<quint32>(buf32.data());
    // read DOS stub
    src.seek(0);
    dosStub->data = src.read(dosSz);
    // read signature (should be "PE\0\0", AKA 0x50450000)
    src.read(buf32.data(), sizeof(quint32));
    if (qFromLittleEndian<quint32>(buf32.data()) != 0x50450000) {
        SET_ERROR_INFO(BadPEFile, InvalidSignature)
        return false;
    }
    // read COFF header
    QByteArray coffDat = src.read(0x14);
    if (!coffHead->read(coffDat, errinfo))
        return false;
    // read optional header
    QByteArray optDat = src.read(coffHead->optHeadSize);
    if (!optHead->read(optDat, errinfo))
        return false;

    // TODO section headers (and sections)

    return true;
}

QByteArray QExe::toBytes()
{
    QByteArray buf32(sizeof(quint32), 0);
    QByteArray out;
    QBuffer dst(&out);
    dst.open(QBuffer::WriteOnly);

    // write DOS stub
    dst.write(dosStub->data);
    // write "PE\0\0" signature
    qToBigEndian<quint32>(0x50450000, buf32.data()); // ???
    dst.write(buf32);
    // convert optional header to bytes
    QByteArray optDat = optHead->toBytes();
    // update COFF header's "optional header size" field
    coffHead->optHeadSize = static_cast<quint16>(optDat.size());
    // write COFF header
    dst.write(coffHead->toBytes());
    // write optional header
    dst.write(optDat);

    // TODO section headers (and sections)

    dst.close();
    return out;
}
