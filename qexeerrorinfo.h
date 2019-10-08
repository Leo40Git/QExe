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
        // BadPEFile
        BadPEFile_InvalidSignature = 1 * 0x100,
        BadPEFile_InvalidSectionCount,
        BadPEFile_InvalidMagic,
    };
    Q_ENUM(ErrorID)
    ErrorID errorID;
    QVariantList details;
};

#endif // QEXEERRORINFO_H
