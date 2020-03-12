#include "qexecoffheader.h"

#include <QDataStream>

#define SET_ERROR_INFO(errName) \
    if (errinfo != nullptr) { \
        errinfo->errorID = QExeErrorInfo::errName; \
    }

quint32 QExeCOFFHeader::size() const
{
    return 0x14;
}

QExeCOFFHeader::QExeCOFFHeader(QExe *exeDat, QObject *parent) : QObject(parent)
{
    this->exeDat = exeDat;

    machineType = I386;
}

bool QExeCOFFHeader::read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo)
{
    (void)src;
    ds >> machineType;
    ds >> sectionCount;
    // "Note that the Windows loader limits the number of sections to 96." (https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image)
    if (sectionCount > 96) {
        SET_ERROR_INFO(BadPEFile_InvalidSectionCount)
        errinfo->details += sectionCount;
        return false;
    }
    ds >> timestamp;
    ds >> symTblPtr;
    ds >> symTblCount;
    ds >> optHeadSize;
    ds >> characteristics;
    return true;
}

bool QExeCOFFHeader::write(QIODevice &dst, QDataStream &ds, QExeErrorInfo *errinfo)
{
    (void)dst, (void)errinfo;
    ds << machineType;
    ds << sectionCount;
    ds << timestamp;
    ds << symTblPtr;
    ds << symTblCount;
    ds << optHeadSize;
    ds << characteristics;
    return true;
}
