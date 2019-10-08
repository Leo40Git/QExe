#ifndef QEXE_H
#define QEXE_H

#include <QObject>
#include <QIODevice>

#include "QExe_global.h"
#include "errorinfo.h"
#include "qexedosstub.h"
#include "qexecoffheader.h"
#include "qexeoptheader.h"

class QEXE_EXPORT QExe : QObject
{
    Q_OBJECT
public:
    explicit QExe(QObject *parent = nullptr);
    void reset();
    bool read(QIODevice &src, ErrorInfo::ErrorInfoStruct *error = nullptr);
    QByteArray toBytes();
private:
    QExeDOSStub *dosStub;
    QExeCOFFHeader *coffHead;
    QExeOptHeader *optHead;
};

#endif // QEXE_H
