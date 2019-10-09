#ifndef QEXESECTIONMANAGER_H
#define QEXESECTIONMANAGER_H

#include <QObject>
#include <QIODevice>
#include <QVector>
#include <QSharedPointer>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexesection.h"

#ifndef QEXE_H
class QExe;
#endif

class QEXE_EXPORT QExeSectionManager : public QObject
{
    Q_OBJECT
    friend class QExe;
public:
    inline quint32 headerSize();
    QVector<QSharedPointer<QExeSection>> sections;
private:
    explicit QExeSectionManager(QExe *exeDat, QObject *parent = nullptr);
    QExe *m_exeDat;
    void read(QIODevice &src);
    void write(QIODevice &dst);
    bool test(QExeErrorInfo *errinfo);
};

#endif // QEXESECTIONMANAGER_H
