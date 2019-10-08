#ifndef QEXE_H
#define QEXE_H

#include <QObject>
#include <QIODevice>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexedosstub.h"
#include "qexecoffheader.h"
#include "qexeoptionalheader.h"
#include "qexesectionmanager.h"

class QEXE_EXPORT QExe : QObject
{
    Q_OBJECT
public:
    static quint32 alignForward(quint32 val, quint32 align);
    explicit QExe(QObject *parent = nullptr);
    void reset();
    bool read(QIODevice &src, QExeErrorInfo *error = nullptr);
    QByteArray toBytes();
    QSharedPointer<QExeDOSStub> dosStub();
    QSharedPointer<QExeCOFFHeader> coffHeader();
    QSharedPointer<QExeOptionalHeader> optionalHeader();
    QSharedPointer<QExeSectionManager> sectionManager();
private:
    void updateComponents();
    QSharedPointer<QExeDOSStub> m_dosStub;
    QSharedPointer<QExeCOFFHeader> m_coffHead;
    QSharedPointer<QExeOptionalHeader> m_optHead;
    QSharedPointer<QExeSectionManager> m_secMgr;
};

#endif // QEXE_H
