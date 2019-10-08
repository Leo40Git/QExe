#include "qexe.h"

#include <QtEndian>
#include <QBuffer>

QExe::QExe(QObject *parent) : QObject(parent)
{
    reset();
}

void QExe::reset()
{
    m_dosStub = new QExeDOSStub(this);
    m_coffHead = new QExeCOFFHeader(this);
    m_optHead = new QExeOptHeader(this);
}

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }

bool QExe::read(QIODevice &src, QExeErrorInfo *errinfo)
{
    // make sure src is usable
    if (!src.isReadable()) {
        SET_ERROR_INFO(BadIODevice_Unreadable)
        return false;
    }
    if (src.isSequential()) {
        SET_ERROR_INFO(BadIODevice_Sequential)
        return false;
    }

    // read DOS stub size
    src.seek(0x3C);
    QByteArray dosSzBuf = src.read(sizeof(quint32));
    qint64 dosSz = qFromLittleEndian<quint32>(dosSzBuf.data());
    // read DOS stub
    src.seek(0);
    m_dosStub->data = src.read(dosSz);
    // read signature (should be "PE\0\0")
    QByteArray sig = src.read(4);
    if (sig[0] != 'P' || sig[1] != 'E' || sig[2] != '\0' || sig[3] != '\0') { // curse you, null terminator
        if (errinfo != nullptr) {
            errinfo->errorID = QExeErrorInfo::BadPEFile_InvalidSignature;
            errinfo->details += QChar(sig[0]);
            errinfo->details += QChar(sig[1]);
            errinfo->details += QChar(sig[2]);
            errinfo->details += QChar(sig[3]);
        }
        return false;
    }
    // read COFF header
    QByteArray coffDat = src.read(0x14);
    if (!m_coffHead->read(coffDat, errinfo))
        return false;
    // read optional header
    QByteArray optDat = src.read(m_coffHead->optHeadSize);
    if (!m_optHead->read(optDat, errinfo))
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
    dst.write(m_dosStub->data);
    // write "PE\0\0" signature
    QLatin1String sig("PE\0\0");
    dst.write(sig.data(), 4);
    // update header fields
    QByteArray optDat = m_optHead->toBytes();
    m_coffHead->optHeadSize = static_cast<quint16>(optDat.size());
    // write COFF header
    dst.write(m_coffHead->toBytes());
    // write optional header
    dst.write(optDat);

    // TODO section headers (and sections)

    dst.close();
    return out;
}

QExeDOSStub &QExe::dosStub()
{
    return *m_dosStub;
}

QExeCOFFHeader &QExe::coffHead()
{
    return *m_coffHead;
}

QExeOptHeader &QExe::optHead()
{
    return *m_optHead;
}
