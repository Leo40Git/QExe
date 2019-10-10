#ifndef QEXESECTION_H
#define QEXESECTION_H

#include "QExe_global.h"
#include "qexeerrorinfo.h"

#ifndef QEXESECTIONMANAGER_H
class QExeSectionManager;
#endif

class QEXE_EXPORT QExeSection : public QObject
{
    Q_OBJECT
    friend class QExeSectionManager;
public:
    enum Characteristic : quint32 {
        Reserved_00000 = 0x00000000, // Reserved for future use.
        Reserved_00001 = 0x00000001, // Reserved for future use.
        Reserved_00002 = 0x00000002, // Reserved for future use.
        Reserved_00004 = 0x00000004, // Reserved for future use.
        DoNotPad = 0x00000008, // The section should not be padded to the next boundary. This flag is obsolete and is replaced by AlignOn1ByteBoundary. This is valid only for object files.
        Reserved_00010 = 0x00000010, // Reserved for future use.
        ContainsCode = 0x00000020, // The section contains executable code.
        ContainsInitializedData = 0x00000040, // The section contains initialized data.
        ContainsUninitializedData = 0x00000080, // The section contains uninitialized data.
        Reserved_0100 = 0x00000100, // Reserved for future use.
        ContainsInfo = 0x00000200, // The section contains comments or other information. The .drectve section has this type. This is valid for object files only.
        Reserved_0400 = 0x00000400, // Reserved for future use.
        WillBeRemoved = 0x00000800, // The section will not become part of the image. This is valid only for object files.
        ContainsCOMDAT = 0x00001000, // The section contains COMDAT data. For more information, see COMDAT Sections (Object Only). This is valid only for object files.
        ReferencedByGP = 0x00008000, // The section contains data referenced through the global pointer (GP).
        Reserved_20000 = 0x00020000, // Reserved for future use. (note: this is 2 flags in the docs?)
        Reserved_40000 = 0x00040000, // Reserved for future use.
        Reserved_80000 = 0x00080000, // Reserved for future use.
        AlignOn1ByteBoundary = 0x00100000, // Align data on a 1-byte boundary. Valid only for object files.
        AlignOn2ByteBoundary = 0x00200000, // Align data on a 2-byte boundary. Valid only for object files.
        AlignOn4ByteBoundary = 0x00300000, // Align data on a 4-byte boundary. Valid only for object files.
        AlignOn8ByteBoundary = 0x00400000, // Align data on an 8-byte boundary. Valid only for object files.
        AlignOn16ByteBoundary = 0x00500000, // Align data on a 16-byte boundary. Valid only for object files.
        AlignOn32ByteBoundary = 0x00600000, // Align data on a 32-byte boundary. Valid only for object files.
        AlignOn64ByteBoundary = 0x00700000, // Align data on a 64-byte boundary. Valid only for object files.
        AlignOn128ByteBoundary = 0x00800000, // Align data on a 128-byte boundary. Valid only for object files.
        AlignOn256ByteBoundary = 0x00900000, // Align data on a 256-byte boundary. Valid only for object files.
        AlignOn512ByteBoundary = 0x00A00000, // Align data on a 512-byte boundary. Valid only for object files.
        AlignOn1024ByteBoundary = 0x00B00000, // Align data on a 1024-byte boundary. Valid only for object files.
        AlignOn2048ByteBoundary = 0x00C00000, // Align data on a 2048-byte boundary. Valid only for object files.
        AlignOn4096ByteBoundary = 0x00D00000, // Align data on a 4096-byte boundary. Valid only for object files.
        AlignOn8192ByteBoundary = 0x00E00000, // Align data on an 8192-byte boundary. Valid only for object files.
        ContainsExtendedRelocs = 0x01000000, // The section contains extended relocations.
        CanBeDiscarded = 0x02000000, // The section can be discarded as needed.
        DoNotCache = 0x04000000, // The section cannot be cached.
        DoNotPage = 0x08000000, // The section is not pageable.
        IsShareable = 0x10000000, // The section can be shared in memory.
        IsExecutable = 0x20000000, // The section can be executed as code.
        IsReadable = 0x40000000, // The section can be read.
        IsWritable = 0x80000000, // The section can be written to.
    };
    Q_DECLARE_FLAGS(Characteristics, Characteristic)
    Q_FLAG(Characteristics)
    bool linearize;
    QLatin1String name() const;
    void setName(const QLatin1String &name);
    quint32 virtualSize;
    quint32 virtualAddr;
    QByteArray rawData;
    quint32 relocsPtr;
    quint32 linenumsPtr;
    quint16 relocsCount;
    quint16 linenumsCount;
    Characteristics characteristics;
private:
    explicit QExeSection(QObject *parent = nullptr);
    QByteArray nameBytes;
    quint32 rawDataPtr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QExeSection::Characteristics)

#endif // QEXESECTION_H
