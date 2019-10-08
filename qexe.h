#ifndef QEXE_H
#define QEXE_H

#include <QObject>
#include <QIODevice>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexedosstub.h"
#include "qexecoffheader.h"
#include "qexeoptheader.h"

class QEXE_EXPORT QExe : QObject
{
    Q_OBJECT
public:
    explicit QExe(QObject *parent = nullptr);
    void reset();
    bool read(QIODevice &src, QExeErrorInfo *error = nullptr);
    QByteArray toBytes();
    QSharedPointer<QExeDOSStub> dosStub();
    QSharedPointer<QExeCOFFHeader> coffHead();
    QSharedPointer<QExeOptHeader> optHead();
private:
    QSharedPointer<QExeDOSStub> m_dosStub;
    QSharedPointer<QExeCOFFHeader> m_coffHead;
    QSharedPointer<QExeOptHeader> m_optHead;
};

#endif // QEXE_H
