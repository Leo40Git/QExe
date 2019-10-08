#include "qexe.h"

#include <QtEndian>
#include <QBuffer>

quint32 QExe::alignForward(quint32 val, quint32 align) {
    quint32 mod = val & align;
    if (mod != 0)
        val += align - mod;
    return val;
}

QExe::QExe(QObject *parent) : QObject(parent)
{
    reset();
}

void QExe::reset()
{
    m_dosStub = QSharedPointer<QExeDOSStub>(new QExeDOSStub(this));
    m_coffHead = QSharedPointer<QExeCOFFHeader>(new QExeCOFFHeader(this));
    m_optHead = QSharedPointer<QExeOptionalHeader>(new QExeOptionalHeader(this));
    m_secMgr = QSharedPointer<QExeSectionManager>(new QExeSectionManager(this));
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
    // section manager taking over to read sections
    m_secMgr->read(src, m_coffHead->sectionCount);

    return true;
}

QByteArray QExe::toBytes()
{
    updateComponents();

    QByteArray buf32(sizeof(quint32), 0);
    QByteArray out;
    QBuffer dst(&out);
    dst.open(QBuffer::WriteOnly);

    // write DOS stub
    dst.write(m_dosStub->data);
    // write "PE\0\0" signature
    QLatin1String sig("PE\0\0");
    dst.write(sig.data(), 4);
    // write COFF header
    dst.write(m_coffHead->toBytes());
    // write optional header
    dst.write(m_optHead->toBytes());
    // section manager taking over to write sections
    m_secMgr->write(dst, m_optHead->fileAlign);

    dst.close();
    return out;
}

QSharedPointer<QExeDOSStub> QExe::dosStub()
{
    return QSharedPointer<QExeDOSStub>(m_dosStub);
}

QSharedPointer<QExeCOFFHeader> QExe::coffHeader()
{
    return QSharedPointer<QExeCOFFHeader>(m_coffHead);
}

QSharedPointer<QExeOptionalHeader> QExe::optionalHeader()
{
    return QSharedPointer<QExeOptionalHeader>(m_optHead);
}

QSharedPointer<QExeSectionManager> QExe::sectionManager()
{
    return QSharedPointer<QExeSectionManager>(m_secMgr);
}

void QExe::updateComponents()
{
    m_coffHead->optHeadSize = static_cast<quint16>(m_optHead->size());
    m_optHead->headerSize = m_dosStub->size() + m_coffHead->size() + m_optHead->size();
    m_optHead->codeSize = 0;
    m_optHead->initializedDataSize = 0;
    m_optHead->uninitializedDataSize = 0;
    m_optHead->imageSize = 0;
    QSharedPointer<QExeSection> section;
    foreach (section, m_secMgr->sections) {
        quint32 v = section->virtualAddr + section->virtualSize;
        if (v > m_optHead->imageSize)
            m_optHead->imageSize = v;
        if (QString(".text").compare(section->name()) == 0)
            m_optHead->codeBaseAddr = section->virtualAddr;
        else if (QString(".rdata").compare(section->name()) == 0)
            m_optHead->dataBaseAddr = section->virtualAddr;
        if (section->characteristics.testFlag(QExeSection::ContainsCode))
            m_optHead->codeSize += section->virtualSize;
        if (section->characteristics.testFlag(QExeSection::ContainsInitializedData))
            m_optHead->initializedDataSize += section->virtualSize;
        if (section->characteristics.testFlag(QExeSection::ContainsUninitializedData))
            m_optHead->uninitializedDataSize += section->virtualSize;
    }
    m_optHead->imageSize = alignForward(m_optHead->imageSize, m_optHead->sectionAlign);
}
