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

    linkerVer.first = 2;
    linkerVer.second = 0x38;
    minOSVer = subsysVer = OSVersion::Windows95;
    subsystem = WinCUI;
    stackReserveSize = 0x200000;
    stackCommitSize = 0x1000;
    heapReserveSize = 0x100000;
    heapCommitSize = 0x1000;
}

bool QExeOptionalHeader::read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo) {
    (void)src;
    // check if this is a PE32 file
    quint16 magic;
    ds >> magic;
    if (magic != 0x10B) {
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
    ds >> dataBaseAddr;
    // Windows only
    ds >> imageBase;
    ds >> sectionAlign;
    ds >> fileAlign;
    ds >> minOSVer;
    ds >> imageVer;
    ds >> subsysVer;
    ds >> win32VerValue;
    ds >> imageSize;
    ds >> headerSize;
    ds >> checksum;
    ds >> subsystem;
    quint16 dllCharsRaw;
    ds >> dllCharsRaw;
    dllCharacteristics = static_cast<DLLCharacteristics>(dllCharsRaw);
    ds >> stackReserveSize;
    ds >> stackCommitSize;
    ds >> heapReserveSize;
    ds >> heapCommitSize;
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
    ds << static_cast<quint16>(0x10B);
    ds << linkerVer;
    ds << codeSize;
    ds << initializedDataSize;
    ds << uninitializedDataSize;
    ds << entryPointAddr;
    ds << codeBaseAddr;
    ds << dataBaseAddr;
    // Windows specific
    ds << imageBase;
    ds << sectionAlign;
    ds << fileAlign;
    ds << minOSVer;
    ds << imageVer;
    ds << subsysVer;
    ds << win32VerValue;
    ds << imageSize;
    ds << headerSize;
    ds << checksum;
    ds << subsystem;
    ds << static_cast<quint16>(dllCharacteristics);
    ds << stackReserveSize;
    ds << stackCommitSize;
    ds << heapReserveSize;
    ds << heapCommitSize;
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
