#ifndef QEXEERRORINFO_H
#define QEXEERRORINFO_H

#include <QObject>
#include <QVariantList>

#include "QExe_global.h"

class QEXE_EXPORT QExeErrorInfo : public QObject
{
    Q_OBJECT
public:
    explicit QExeErrorInfo(QObject *parent = nullptr) : QObject(parent) {}
    enum ErrorID : quint32 {
        // BadIODevice
        BadIODevice_Unreadable = 0 * 0x100,
        BadIODevice_Sequential,
        BadIODevice_Unwritable,
        // BadPEFile
        BadPEFile_InvalidSignature = 1 * 0x100,
        BadPEFile_InvalidSectionCount,
        BadPEFile_InvalidMagic,
        // BadSection
        BadSection_VirtualOverlap = 2 * 0x100,
        BadSection_LinearizeFailure,
        // BadRsrc
        BadRsrc_InvalidFormat = 3 * 0x100,
    };
    Q_ENUM(ErrorID)
    ErrorID errorID;
    QVariantList details;
};

#endif // QEXEERRORINFO_H
