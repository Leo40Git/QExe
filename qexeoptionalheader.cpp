#include "qexeoptionalheader.h"

#include <QBuffer>
#include <QtEndian>

#define READ_VERSION(name, size) \
    buf.read(buf ## size.data(), sizeof(quint ## size)); \
    name ## Ver.first = qFromLittleEndian<quint ## size>(buf ## size.data()); \
    buf.read(buf ## size.data(), sizeof(quint ## size)); \
    name ## Ver.second = qFromLittleEndian<quint ## size>(buf ## size.data());

#define WRITE_VERSION(name, size) \
    qToLittleEndian<quint ## size>(name ## Ver.first, buf ## size.data()); \
    buf.write(buf ## size); \
    qToLittleEndian<quint ## size>(name ## Ver.second, buf ## size.data()); \
    buf.write(buf ## size);

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }


quint32 QExeOptionalHeader::size() const
{
    // 0x1C (standard) + 0x44 (Windows specific) + dataDirectories.size() * 8
    return 0x60 + static_cast<quint32>(dataDirectories.size()) * 8;
}

QExeOptionalHeader::QExeOptionalHeader(QExe *exeDat, QObject *parent) : QObject(parent)
{
    this->exeDat = exeDat;

    linkerVer.first = 2;
    linkerVer.second = 0x38;
    minOSVer = OSVersion_Win95;
    subsysVer = OSVersion_Win95;
    subsystem = WinCUI;
    stackReserveSize = 0x200000;
    stackCommitSize = 0x1000;
    heapReserveSize = 0x100000;
    heapCommitSize = 0x1000;
}

bool QExeOptionalHeader::read(QByteArray src, QExeErrorInfo *errinfo) {
    QByteArray buf8(sizeof(quint8), 0);
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    QByteArray buf64(sizeof(quint64), 0);

    QBuffer buf(&src);
    buf.open(QBuffer::ReadOnly);
    // Standard
    buf.read(buf16.data(), sizeof(quint16));
    quint16 magic = qFromLittleEndian<quint16>(buf16.data());
    if (magic != 0x10B) {
        if (errinfo != nullptr) {
            errinfo->errorID = QExeErrorInfo::BadPEFile_InvalidMagic;
            errinfo->details += magic;
        }
        return false;
    }
    READ_VERSION(linker, 8)
    buf.read(buf32.data(), sizeof(quint32));
    codeSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    initializedDataSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    uninitializedDataSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    entryPointAddr = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    codeBaseAddr = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    dataBaseAddr = qFromLittleEndian<quint32>(buf32.data());
    // Windows only
    buf.read(buf32.data(), sizeof(quint32));
    imageBase = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    sectionAlign = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    fileAlign = qFromLittleEndian<quint32>(buf32.data());
    READ_VERSION(minOS, 16)
    READ_VERSION(image, 16)
    READ_VERSION(subsys, 16)
    buf.read(buf32.data(), sizeof(quint32));
    win32VerValue = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    imageSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    headerSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    checksum = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf16.data(), sizeof(quint16));
    subsystem = static_cast<Subsystem>(qFromLittleEndian<quint16>(buf16.data()));
    buf.read(buf16.data(), sizeof(quint16));
    dllCharacteristics = DLLCharacteristics(qFromLittleEndian<quint16>(buf16.data()));
    buf.read(buf32.data(), sizeof(quint32));
    stackReserveSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    stackCommitSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    heapReserveSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    heapCommitSize = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    loaderFlags = qFromLittleEndian<quint32>(buf32.data());
    buf.read(buf32.data(), sizeof(quint32));
    quint32 dirCount = qFromLittleEndian<quint32>(buf32.data());
    // Data directories
    dataDirectories.clear();
    for (quint32 i = 0; i < dirCount; i++) {
        buf.read(buf32.data(), sizeof(quint32));
        quint32 rva = qFromLittleEndian<quint32>(buf32.data());
        buf.read(buf32.data(), sizeof(quint32));
        quint32 size = qFromLittleEndian<quint32>(buf32.data());
        dataDirectories += DataDirectoryPtr(new DataDirectory(rva, size));
    }
    buf.close();
    return true;
}

QByteArray QExeOptionalHeader::toBytes()
{
    QByteArray out(static_cast<int>(size()), 0);

    QByteArray buf8(sizeof(quint8), 0);
    QByteArray buf16(sizeof(quint16), 0);
    QByteArray buf32(sizeof(quint32), 0);
    QByteArray buf64(sizeof(quint64), 0);

    QBuffer buf(&out);
    buf.open(QBuffer::WriteOnly);
    // Standard
    qToLittleEndian<quint16>(0x10B, buf16.data());
    buf.write(buf16);
    WRITE_VERSION(linker, 8)
    qToLittleEndian<quint32>(codeSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(initializedDataSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(uninitializedDataSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(entryPointAddr, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(codeBaseAddr, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(dataBaseAddr, buf32.data());
    buf.write(buf32);
    // Windows specific
    qToLittleEndian<quint32>(imageBase, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(sectionAlign, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(fileAlign, buf32.data());
    buf.write(buf32);
    WRITE_VERSION(minOS, 16)
    WRITE_VERSION(image, 16)
    WRITE_VERSION(subsys, 16)
    qToLittleEndian<quint32>(win32VerValue, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(imageSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(headerSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(checksum, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint16>(subsystem, buf16.data());
    buf.write(buf16);
    qToLittleEndian<quint16>(static_cast<quint16>(dllCharacteristics), buf16.data());
    buf.write(buf16);
    qToLittleEndian<quint32>(stackReserveSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(stackCommitSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(heapReserveSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(heapCommitSize, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(loaderFlags, buf32.data());
    buf.write(buf32);
    qToLittleEndian<quint32>(static_cast<quint32>(dataDirectories.size()), buf32.data());
    buf.write(buf32);
    // Data directories
    DataDirectoryPtr pair;
    foreach (pair, dataDirectories) {
        qToLittleEndian<quint32>(pair->first, buf32.data());
        buf.write(buf32);
        qToLittleEndian<quint32>(pair->second, buf32.data());
        buf.write(buf32);
    }
    buf.close();
    return out;
}

#undef READ_VERSION
#undef WRITE_VERSION
