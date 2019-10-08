#ifndef ERRORINFO_H
#define ERRORINFO_H

#include <QObject>
#include <QVariantList>

#include "QExe_global.h"

namespace ErrorInfo {
    Q_NAMESPACE

    enum ErrorID {
        BadIODevice = 0,
        BadPEFile
    };

    Q_ENUM_NS(ErrorID)

    enum SubErrorID {
        // BadIODevice
        BadIODevice_Unreadable = BadIODevice * 0x100,
        BadIODevice_Sequential,
        // BadPEFile
        BadPEFile_InvalidSignature = BadPEFile * 0x100,
        BadPEFile_InvalidSectionCount,
        BadPEFile_InvalidMagic,
    };
    Q_ENUM_NS(SubErrorID)

    struct ErrorInfoStruct {
        ErrorID errorID;
        SubErrorID subID;
        QVariantList details;
    };
}

#endif // ERRORINFO_H
