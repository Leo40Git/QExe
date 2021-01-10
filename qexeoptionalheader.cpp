#include "qexeoptionalheader.h"

#include <QDataStream>

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

    // Standard
    isPlus = false;
    linkerVer.first = 2;
    linkerVer.second = 0x38;
    entryPointAddr = 0;
    codeBaseAddr = 0;
    dataBaseAddr = 0;
    // Windows only
    imageBase = 0x400000;
    sectionAlign = 0x100;
    fileAlign = 0x100;
    minOSVer = subsysVer = OSVersion::Windows95;
    imageVer = Version16(0, 0);
    subsysVer = Version16(0, 0);
    win32VerValue = 0;
    checksum = 0;
    subsystem = WinCUI;
    dllCharacteristics = DLLCharacteristics();
    stackReserveSize = 0x200000;
    stackCommitSize = 0x1000;
    heapReserveSize = 0x100000;
    heapCommitSize = 0x1000;
    loaderFlags = 0;
    // Data directories
    dataDirectories.clear();
}

bool QExeOptionalHeader::read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo) {
    (void)src;
    // check if this is a PE32(+) file
    quint16 magic;
    ds >> magic;
    isPlus = magic == 0x20B;
    if (magic != 0x10B && !isPlus) {
        if (errinfo != nullptr) {
            errinfo->errorID = QExeErrorInfo::BadPEFile_InvalidMagic;
            errinfo->details += magic;
        }
        return false;

    }
    // Standard
    ds >> linkerVer;
    ds >> codeSize;
    ds >> initializedDataSize;
    ds >> uninitializedDataSize;
    ds >> entryPointAddr;
    ds >> codeBaseAddr;
    if (!isPlus)
        ds >> dataBaseAddr;
    // Windows only
    if (isPlus)
        ds >> imageBase;
    else {
        imageBase = 0;
        ds >> (quint32 &) imageBase;
    }
    ds >> sectionAlign;
    ds >> fileAlign;
    ds >> minOSVer;
    ds >> imageVer;
    ds >> subsysVer;
    ds >> win32VerValue;
    ds >> imageSize;
    ds >> headerSize;
    ds >> checksum;
    ds >> (quint16 &) subsystem;
    quint16 dllCharsRaw;
    ds >> dllCharsRaw;
    dllCharacteristics = static_cast<DLLCharacteristics>(dllCharsRaw);
    if (isPlus) {
        ds >> stackReserveSize;
        ds >> stackCommitSize;
        ds >> heapReserveSize;
        ds >> heapCommitSize;
    } else {
        stackReserveSize = 0;
        ds >> (quint32 &) stackReserveSize;
        stackCommitSize = 0;
        ds >> (quint32 &) stackCommitSize;
        heapReserveSize = 0;
        ds >> (quint32 &) heapReserveSize;
        heapCommitSize = 0;
        ds >> (quint32 &) heapCommitSize;
    }
    ds >> loaderFlags;
    quint32 dirCount;
    ds >> dirCount;
    // Data directories
    dataDirectories.clear();
    quint32 rva, size;
    for (quint32 i = 0; i < dirCount; i++) {
        ds >> rva;
        ds >> size;
        dataDirectories += DataDirectoryPtr(new DataDirectory(rva, size));
    }
    return true;
}

bool QExeOptionalHeader::write(QIODevice &dst, QDataStream &ds, QExeErrorInfo *errinfo)
{
    (void)dst, (void)errinfo;
    ds << static_cast<quint16>(isPlus ? 0x20B : 0x10B);
    ds << linkerVer;
    ds << codeSize;
    ds << initializedDataSize;
    ds << uninitializedDataSize;
    ds << entryPointAddr;
    ds << codeBaseAddr;
    if (!isPlus)
        ds << dataBaseAddr;
    // Windows specific
    if (isPlus)
        ds << imageBase;
    else
        ds << static_cast<quint32>(imageBase);
    ds << sectionAlign;
    ds << fileAlign;
    ds << minOSVer;
    ds << imageVer;
    ds << subsysVer;
    ds << win32VerValue;
    ds << imageSize;
    ds << headerSize;
    ds << checksum;
    ds << (quint16 &) subsystem;
    ds << static_cast<quint16>(dllCharacteristics);
    if (isPlus) {
        ds << stackReserveSize;
        ds << stackCommitSize;
        ds << heapReserveSize;
        ds << heapCommitSize;
    } else {
        ds << static_cast<quint32>(stackReserveSize);
        ds << static_cast<quint32>(stackCommitSize);
        ds << static_cast<quint32>(heapReserveSize);
        ds << static_cast<quint32>(heapCommitSize);
    }
    ds << loaderFlags;
    ds << static_cast<quint32>(dataDirectories.size());
    // Data directories
    DataDirectoryPtr pair;
    foreach (pair, dataDirectories) {
        ds << *pair;
    }
    return true;
}

#undef READ_VERSION
#undef WRITE_VERSION
