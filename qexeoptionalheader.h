#ifndef QEXEOPTIONALHEADER_H
#define QEXEOPTIONALHEADER_H

#include <QIODevice>
#include <QSharedPointer>

#include "QExe_global.h"
#include "qexeerrorinfo.h"

class QExe;
class QExeCOFFHeader;
class QExeSectionManager;

#include "typedef_version.h"
typedef QPair<quint32, quint32> DataDirectory; // first => address, second => size
typedef QSharedPointer<DataDirectory> DataDirectoryPtr;

class QEXE_EXPORT QExeOptionalHeader : public QObject
{
    Q_OBJECT
public:
    // https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#windows-subsystem
    enum Subsystem : quint16 {
        Unknown = 0,
        Native = 1, // Device driver or native Windows process
        WinGUI = 2,
        WinCUI = 3, // Windows character subsystem
        OS2CUI = 5, // OS/2 character subsystem
        PosixCUI = 7, // Posix character subsystem
        NativeWin = 8, // Native Win9x driver
        WinCEGUI = 9,
        EFIApp = 10,
        EFIBootDriver = 11,
        EFIRuntimeDriver = 12,
        EFIROM = 13,
        Xbox = 14,
        WinBootApp = 16
    };
    Q_ENUM(Subsystem)
    // https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#dll-characteristics
    enum DLLCharacteristic : quint16 {
        Reserved_0001 = 0x0001, // Reserved, must be zero.
        Reserved_0002 = 0x0002, // Reserved, must be zero.
        Reserved_0004 = 0x0004, // Reserved, must be zero.
        Reserved_0008 = 0x0008, // Reserved, must be zero.
        HighEntropyVA = 0x0020, // Image can handle a high entropy 64-bit virtual address space.
        DynamicBase = 0x0040, // DLL can be relocated at load time.
        ForceCodeIntegrity = 0x0080, // Code Integrity checks are enforced.
        NXCompatible = 0x0100, // Image is NX compatible.
        NoIsolation = 0x0200, // Isolation aware, but do not isolate the image.
        NoSEHandling = 0x0400, // Does not use structured exception (SE) handling. No SE handler may be called in this image.
        NoBinding = 0x0800, // Do not bind the image.
        RunInAppContainer = 0x1000, // Image must execute in an AppContainer.
        IsWDMDriver = 0x2000, // A WDM driver.
        SupportsCFGuard = 0x4000, // Image supports Control Flow Guard.
        TerminalServerAware = 0x8000, // Terminal Server aware.
    };
    Q_DECLARE_FLAGS(DLLCharacteristics, DLLCharacteristic)
    Q_FLAG(DLLCharacteristics)

    quint32 size() const;
    // Standard
    bool isPlus;
    Version8 linkerVer;
    quint32 entryPointAddr;
    quint32 codeBaseAddr;
    quint32 dataBaseAddr; // present in PE32, not in PE32+
    // Windows only
    quint64 imageBase; // quint32 in PE32
    quint32 sectionAlign;
    quint32 fileAlign;
    Version16 minOSVer;
    Version16 imageVer;
    Version16 subsysVer;
    quint32 win32VerValue; // reserved, must be 0
    quint32 checksum;
    Subsystem subsystem;
    DLLCharacteristics dllCharacteristics;
    quint64 stackReserveSize; // quint32 in PE32
    quint64 stackCommitSize; // quint32 in PE32
    quint64 heapReserveSize; // quint32 in PE32
    quint64 heapCommitSize; // quint32 in PE32
    quint32 loaderFlags; // reserved, must be 0
    // https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
    enum DataDirectories : int {
        ExportTable = 0,
        ImportTable,
        ResourceTable,
        ExceptionTable,
        CertificateTable,
        BaseRelocationTable,
        DebugData,
        Reserved_07, // both must be 0
        GlobalPointer, // size must be 0
        ThreadLocalStorageTable,
        LoadConfigTable,
        BoundImportTable,
        ImportAddrTable,
        DelayImportDescriptor,
        CLRRuntimeHeader,
        Reserved_15 // both must be 0
    };
    Q_ENUM(DataDirectories)
    QList<DataDirectoryPtr> dataDirectories;

private:
    friend class QExe;
    friend class QExeSectionManager;

    explicit QExeOptionalHeader(QExe *exeDat, QObject *parent = nullptr);
    QExe *exeDat;
    bool read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo);
    bool write(QIODevice &dst, QDataStream &ds, QExeErrorInfo *errinfo);
    // managed by QExe
    quint32 codeSize;
    quint32 initializedDataSize;
    quint32 uninitializedDataSize;
    quint32 imageSize;
    quint32 headerSize;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QExeOptionalHeader::DLLCharacteristics)

#endif // QEXEOPTIONALHEADER_H
