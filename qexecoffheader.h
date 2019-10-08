#ifndef QEXEHEADER_H
#define QEXEHEADER_H

#include <QObject>

#include "QExe_global.h"
#include "qexeerrorinfo.h"

#ifndef QEXE_H
class QExe;
#endif

class QEXE_EXPORT QExeCOFFHeader : public QObject
{
    Q_OBJECT
    friend class QExe;
public:
    // https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
    enum MachineType : quint16 {
        Any = 0x0,
        AM33 = 0x1D3, // Matsushita AM33
        AMD64 = 0x8664, // x64
        ARM = 0x1C0, // ARM little endian
        ARM64 = 0xAA64, // ARM64 little endian
        ARMNT = 0x1C4, // ARM Thumb-2 little endian
        EBC = 0xEBC, // EFI byte code
        I386 = 0x14C, // Intel 386 or later processors and compatible processors
        IA64 = 0x200, // Intel Itanium processor family
        M32R = 0x9041, // Mitsubishi M32R little endian
        MIPS16 = 0x266, // MIPS16
        MIPSFPU = 0x366, // MIPS with FPU
        MIPSFPU16 = 0x466, // MIPS16 with FPU
        PowerPC = 0x1F0, // Power PC little endian
        PowerPCFP = 0x1F1, // Power PC with floating point support
        R4000 = 0x166, // MIPS little endian
        RISCV32 = 0x5032, // RISC-V 32-bit address space
        RISCV64 = 0x5064, // RISC-V 64-bit address space
        RISCV128 = 0x5128, // RISC-V 128-bit address space
        SH3 = 0x1A2, // Hitachi SH3
        SH3DSP = 0x1A3, // Hitachi SH3 DSP
        SH4 = 0x1A6, // Hitachi SH4
        SH5 = 0x1A8, // Hitachi SH5
        Thumb = 0x1C2,
        WCEMIPSV2 = 0x169, // MIPS little-endian WCE v2
    };
    Q_ENUM(MachineType)
    // https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#characteristics
    enum Characteristic : quint16 {
        RelocsStripped = 0x0001, // Image only, Windows CE, and Microsoft Windows NT and later. This indicates that the file does not contain base relocations and must therefore be loaded at its preferred base address. If the base address is not available, the loader reports an error. The default behavior of the linker is to strip base relocations from executable (EXE) files.
        IsExecutable = 0x0002, // Image only. This indicates that the image file is valid and can be run. If this flag is not set, it indicates a linker error.
        LineNumsStripped = 0x0004, // COFF line numbers have been removed. This flag is deprecated and should be zero.
        LocalSymsStripped = 0x0008, // COFF symbol table entries for local symbols have been removed. This flag is deprecated and should be zero.
        AggresiveWSTrim = 0x0010, // Obsolete. Aggressively trim working set. This flag is deprecated for Windows 2000 and later and must be zero.
        LargeAddressAware = 0x0020, // Application can handle > 2-GB addresses.
        Reserved_0040 = 0x0040, // This flag is reserved for future use.
        IsLittleEndian = 0x0080, // Little endian: the least significant bit (LSB) precedes the most significant bit (MSB) in memory. This flag is deprecated and should be zero.
        x86Machine = 0x0100, // Machine is based on a 32-bit-word architecture.
        DebugInfoStripped = 0x0200, // Debugging information is removed from the image file.
        RemovableRunFromSwap = 0x0400, // If the image is on removable media, fully load it and copy it to the swap file.
        NetRunFromSwap = 0x0800, // If the image is on network media, fully load it and copy it to the swap file.
        IsSystem = 0x1000, // The image file is a system file, not a user program.
        IsDLL = 0x2000, // The image file is a dynamic-link library (DLL). Such files are considered executable files for almost all purposes, although they cannot be directly run.
        UpSystemOnly = 0x4000, // The file should be run only on a uniprocessor machine.
        IsBigEndian = 0x8000, // Big endian: the MSB precedes the LSB in memory. This flag is deprecated and should be zero.
    };
    Q_DECLARE_FLAGS(Characteristics, Characteristic)
    Q_FLAG(Characteristics)
    MachineType machineType;
    quint32 timestamp;
    quint32 symTblPtr;
    quint32 symTblCount;
    Characteristics characteristics;

private:
    explicit QExeCOFFHeader(QObject *parent = nullptr);
    bool read(QByteArray src, QExeErrorInfo *errinfo);
    QByteArray toBytes();
    // these two are managed by QExe
    quint16 sectionCount;
    quint16 optHeadSize;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QExeCOFFHeader::Characteristics)

#endif // QEXEHEADER_H
