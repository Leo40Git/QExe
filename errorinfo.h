#ifndef ERRORINFO_H
#define ERRORINFO_H

#include <QObject>
#include <QVariantList>

namespace ErrorInfo {
    Q_NAMESPACE

    enum ErrorID {
        Success = 0,
        BadIODevice,
        BadPEFile
    };
    Q_ENUM_NS(ErrorID);

    enum SubErrorID {
        // Success
        Success_NoErrors = 0,
        // BadIODevice
        BadIODevice_Unreadable = 0,
        BadIODevice_Sequential,
        // BadPEFile
        BadPEFile_InvalidSignature = 0,
        BadPEFile_InvalidSectionCount,
        BadPEFile_InvalidMagic,
    };
    Q_ENUM_NS(SubErrorID);

    struct ErrorInfoStruct {
        ErrorID errorID;
        SubErrorID subID;
        QVariantList details;
    };
}

#endif // ERRORINFO_H
