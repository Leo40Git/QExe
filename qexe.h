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
    friend class QExeDOSStub;
	friend class QExeCOFFHeader;
    friend class QExeOptionalHeader;
    friend class QExeSectionManager;
public:
    template<typename V>
    static V alignForward(V val, V align) {
        V mod = val % align;
        if (mod != 0)
            val += align - mod;
        return val;
    }
    explicit QExe(QObject *parent = nullptr);
    void reset();
    bool read(QIODevice &src, QExeErrorInfo *error = nullptr);
    bool toBytes(QByteArray &dst, QExeErrorInfo *error = nullptr);
    QSharedPointer<QExeDOSStub> dosStub();
    QSharedPointer<QExeCOFFHeader> coffHeader();
    QSharedPointer<QExeOptionalHeader> optionalHeader();
    QSharedPointer<QExeSectionManager> sectionManager();
private:
    bool updateComponents(QExeErrorInfo *error);
    QSharedPointer<QExeDOSStub> m_dosStub;
    QSharedPointer<QExeCOFFHeader> m_coffHead;
    QSharedPointer<QExeOptionalHeader> m_optHead;
    QSharedPointer<QExeSectionManager> m_secMgr;
};

#endif // QEXE_H
