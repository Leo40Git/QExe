#include "qexe.h"

#include <QtEndian>
#include <QBuffer>

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

bool QExe::dataDirSectionName(QExeOptionalHeader::DataDirectories dataDir, QLatin1String *secName)
{
    if (!secName)
        return false;
    for (auto it = secName2DataDir.constKeyValueBegin() ; it != secName2DataDir.constKeyValueEnd() ; ++it) {
        if ((*it).second == dataDir) {
            *secName = (*it).first;
            return true;
        }
    }
    return false;
}

QExe::QExe(QObject *parent) : QObject(parent)
{
    m_autoCreateRsrcManager = true;
    m_autoAddFillerSections = true;
    reset();
}

void QExe::reset()
{
    m_dosStub = QSharedPointer<QExeDOSStub>(new QExeDOSStub(this));
    m_coffHead = QSharedPointer<QExeCOFFHeader>(new QExeCOFFHeader(this));
    m_optHead = QSharedPointer<QExeOptionalHeader>(new QExeOptionalHeader(this));
    m_secMgr = QSharedPointer<QExeSectionManager>(new QExeSectionManager(this));
    m_rsrcMgr = nullptr;
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

    if (m_autoCreateRsrcManager)
        if (createRsrcManager(errinfo) == nullptr)
            return false;

    return true;
}

bool QExe::toBytes(QByteArray &dst, QExeErrorInfo *errinfo)
{
    quint32 fileSize;
    if (!updateComponents(&fileSize, errinfo))
        return false;

    dst = QByteArray(static_cast<qint32>(fileSize), 0);
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

    buf.close();

    // remove generated .rsrc section
    if (!m_rsrcMgr.isNull()) {
        int rsI = m_secMgr->rsrcSectionIndex();
        if (rsI > 0)
            m_secMgr->removeSection(rsI);
    }

    return true;
}

QSharedPointer<QExeDOSStub> QExe::dosStub()
{
    return m_dosStub;
}

QSharedPointer<QExeCOFFHeader> QExe::coffHeader()
{
    return m_coffHead;
}

QSharedPointer<QExeOptionalHeader> QExe::optionalHeader()
{
    return m_optHead;
}

QSharedPointer<QExeSectionManager> QExe::sectionManager()
{
    return m_secMgr;
}

QSharedPointer<QExeRsrcManager> QExe::rsrcManager()
{
    return m_rsrcMgr;
}

QSharedPointer<QExeRsrcManager> QExe::createRsrcManager(QExeErrorInfo *errinfo)
{
    if (m_rsrcMgr.isNull())
        m_rsrcMgr = QSharedPointer<QExeRsrcManager>(new QExeRsrcManager(this));
    // if we have a .rsrc section, read from it
    int rsI = m_secMgr->rsrcSectionIndex();
    if (rsI > 0) {
        if (!m_rsrcMgr->read(m_secMgr->sectionAt(rsI), errinfo))
            return nullptr;
        m_secMgr->removeSection(rsI);
    }
    return m_rsrcMgr;
}

bool QExe::removeRsrcManager()
{
    if (m_rsrcMgr.isNull())
        return false;
    m_rsrcMgr = nullptr;
    return true;
}

void QExe::updateHeaderSizes()
{
    m_coffHead->optHeadSize = static_cast<quint16>(m_optHead->size());
    m_optHead->headerSize = m_dosStub->size() + m_coffHead->size() + m_optHead->size() + m_secMgr->headerSize();
    if (!m_rsrcMgr.isNull())
        m_optHead->headerSize += m_rsrcMgr->headerSize();
    m_optHead->headerSize = alignForward(m_optHead->headerSize, m_optHead->fileAlign);
}

bool QExe::updateComponents(quint32 *fileSize, QExeErrorInfo *errinfo)
{
    if (!m_rsrcMgr.isNull())
        m_rsrcMgr->toSection();
    if (!m_secMgr->test(false, fileSize, errinfo))
        return false;
    m_optHead->codeSize = 0;
    m_optHead->initializedDataSize = 0;
    m_optHead->uninitializedDataSize = 0;
    m_optHead->imageSize = 0;
    QExeSectionPtr section;
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
    m_optHead->codeSize = alignForward(m_optHead->codeSize, m_optHead->sectionAlign);
    m_optHead->initializedDataSize = alignForward(m_optHead->initializedDataSize, m_optHead->sectionAlign);
    m_optHead->uninitializedDataSize = alignForward(m_optHead->uninitializedDataSize, m_optHead->sectionAlign);
    m_optHead->imageSize = alignForward(m_optHead->imageSize, m_optHead->sectionAlign);
    return true;
}

bool QExe::autoCreateRsrcManager() const
{
    return m_autoCreateRsrcManager;
}

void QExe::setAutoCreateRsrcManager(bool autoCreateRsrcManager)
{
    m_autoCreateRsrcManager = autoCreateRsrcManager;
}

bool QExe::autoAddFillerSections() const
{
    return m_autoAddFillerSections;
}

void QExe::setAutoAddFillerSections(bool autoAddFillerSections)
{
    m_autoAddFillerSections = autoAddFillerSections;
}
