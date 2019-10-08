#ifndef QEXEDOSSTUB_H
#define QEXEDOSSTUB_H

#include <QObject>

#include "QExe_global.h"
#include "qexeerrorinfo.h"

#ifndef QEXE_H
class QExe;
#endif

class QEXE_EXPORT QExeDOSStub : public QObject
{
    Q_OBJECT
    friend class QExe;
public:
    QByteArray data;
private:
    explicit QExeDOSStub(QObject *parent = nullptr);
};

#endif // QEXEDOSSTUB_H
