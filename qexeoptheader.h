#ifndef QEXEOPTHEADER_H
#define QEXEOPTHEADER_H

#include <QObject>

#include "QExe_global.h"
#include "errorinfo.h"

#ifndef QEXE_H
class QExe;
#endif

class QEXE_EXPORT QExeOptHeader : public QObject
{
    Q_OBJECT
    friend class QExe;
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
        XBOX = 14,
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
        NOSEHandling = 0x0400, // Does not use structured exception (SE) handling. No SE handler may be called in this image.
        NoBinding = 0x0800, // Do not bind the image.
        RunInAppContainer = 0x1000, // Image must execute in an AppContainer.
        IsWDMDriver = 0x2000, // A WDM driver.
        SupportsCFGuard = 0x4000, // Image supports Control Flow Guard.
        TerminalServerAware = 0x8000, // Terminal Server aware.
    };
    Q_DECLARE_FLAGS(DLLCharacteristics, DLLCharacteristic)
    Q_FLAG(DLLCharacteristics)
private:
    explicit QExeOptHeader(QObject *parent = nullptr);
    bool read(QByteArray src, ErrorInfo::ErrorInfoStruct *errinfo);
    QByteArray toBytes();
#define DECLARE_VERSION(name, size) quint ## size name ## VerMajor, name ## VerMinor;
    DECLARE_VERSION(linker, 8)
    quint32 codeSize;
    quint32 initializedDataSize;
    quint32 uninitializedDataSize;
    quint32 entryPointAddr;
    quint32 codeBaseAddr;
    quint32 dataBaseAddr;
    quint32 imageBase;
    quint32 sectionAlign;
    quint32 fileAlign;
    DECLARE_VERSION(minOS, 16)
    DECLARE_VERSION(image, 16)
    DECLARE_VERSION(subsys, 16)
    quint32 win32VerValue; // reserved, must be 0
    quint32 imageSize;
    quint32 headerSize;
    quint32 checksum;
    Subsystem subsystem;
    DLLCharacteristics dllCharacteristics;
    quint32 stackReserveSize;
    quint32 stackCommitSize;
    quint32 heapReserveSize;
    quint32 heapCommitSize;
    quint32 loaderFlags; // reserved, must be 0
    QList<QPair<quint32, quint32>> imageDataDirectories;
#undef DECLARE_VERSION
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QExeOptHeader::DLLCharacteristics)

#endif // QEXEOPTHEADER_H