#include "qexe.h"

#include <QtEndian>
#include <QBuffer>

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
    m_secMgr->read(src);

    return true;
}

bool QExe::toBytes(QByteArray &dst, QExeErrorInfo *errinfo)
{
    if (!updateComponents(errinfo))
        return false;

    QByteArray buf32(sizeof(quint32), 0);
    dst = QByteArray();
    QBuffer buf(&dst);
    buf.open(QBuffer::WriteOnly);

    // write DOS stub
    buf.write(m_dosStub->data);
    // write "PE\0\0" signature
    QLatin1String sig("PE\0\0");
    buf.write(sig.data(), 4);
    // write COFF header
    buf.write(m_coffHead->toBytes());
    // write optional header
    buf.write(m_optHead->toBytes());
    // section manager taking over to write sections
    m_secMgr->write(buf);
    // pad to file align
    buf.seek(alignForward(buf.pos(), static_cast<qint64>(m_optHead->fileAlign)));

    buf.close();
    return true;
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

static QMap<QLatin1String, QExeOptionalHeader::DataDirectories> secName2DataDir {
    { QLatin1String(".edata"), QExeOptionalHeader::ExportTable },
    { QLatin1String(".idata"), QExeOptionalHeader::ImportTable },
    { QLatin1String(".rsrc"), QExeOptionalHeader::ResourceTable },
    { QLatin1String(".pdata"), QExeOptionalHeader::ExceptionTable },
    { QLatin1String(".reloc"), QExeOptionalHeader::BaseRelocationTable },
    { QLatin1String(".debug"), QExeOptionalHeader::DebugData },
    { QLatin1String(".tls"), QExeOptionalHeader::ThreadLocalStorageTable },
    { QLatin1String(".cormeta"), QExeOptionalHeader::CLRRuntimeHeader },
};

bool QExe::updateComponents(QExeErrorInfo *errinfo)
{
    m_coffHead->optHeadSize = static_cast<quint16>(m_optHead->size());
    m_optHead->headerSize = alignForward(m_dosStub->size() + m_coffHead->size() + m_optHead->size() + m_secMgr->headerSize(), m_optHead->fileAlign);
    if (!m_secMgr->test(errinfo))
        return false;
    m_optHead->codeSize = 0;
    m_optHead->initializedDataSize = 0;
    m_optHead->uninitializedDataSize = 0;
    m_optHead->imageSize = 0;
    QSharedPointer<QExeSection> section;
    foreach (section, m_secMgr->sections) {
        quint32 v = section->virtualAddr + section->virtualSize;
        if (v > m_optHead->imageSize)
            m_optHead->imageSize = v;
        QLatin1String secName = section->name();
        if (secName2DataDir.contains(secName)) {
            int dirID = secName2DataDir[secName];
            if (dirID < m_optHead->dataDirectories.size()) {
                DataDirectoryPtr rsrcDir = m_optHead->dataDirectories[dirID];
                rsrcDir->first = section->virtualAddr;
                rsrcDir->second = section->virtualSize;
            }
        } else if (QString(".text").compare(secName) == 0)
            m_optHead->codeBaseAddr = section->virtualAddr;
        else if (QString(".rdata").compare(secName) == 0)
            m_optHead->dataBaseAddr = section->virtualAddr;
        if (section->characteristics.testFlag(QExeSection::ContainsCode))
            m_optHead->codeSize += section->virtualSize;
        if (section->characteristics.testFlag(QExeSection::ContainsInitializedData))
            m_optHead->initializedDataSize += section->virtualSize;
        if (section->characteristics.testFlag(QExeSection::ContainsUninitializedData))
            m_optHead->uninitializedDataSize += section->virtualSize;
    }
    m_optHead->imageSize = alignForward(m_optHead->imageSize, m_optHead->sectionAlign);
    return true;
}
