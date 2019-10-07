#ifndef QEXEDOSSTUB_H
#define QEXEDOSSTUB_H

#include <QObject>

#include "QExe_global.h"
#include "errorinfo.h"

#ifndef QEXE_H
class QExe;
#endif

class QEXE_EXPORT QExeDOSStub : public QObject
{
    Q_OBJECT
    friend class QExe;
private:
    explicit QExeDOSStub(QObject *parent = nullptr);
    QByteArray data;
};

#endif // QEXEDOSSTUB_H
