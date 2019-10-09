#ifndef QEXEDOSSTUB_H
#define QEXEDOSSTUB_H

#include <QObject>

#include "QExe_global.h"
#include "qexeerrorinfo.h"

class QExe;
class QExeCOFFHeader;
class QExeOptionalHeader;
class QExeSectionManager;

class QEXE_EXPORT QExeDOSStub : public QObject
{
    Q_OBJECT
    friend class QExe;
    friend class QExeCOFFHeader;
    friend class QExeOptionalHeader;
    friend class QExeSectionManager;
public:
    quint32 size() const;
    QByteArray data;
private:
    explicit QExeDOSStub(QExe *exeDat, QObject *parent = nullptr);
    QExe *m_exeDat;
};

#endif // QEXEDOSSTUB_H
