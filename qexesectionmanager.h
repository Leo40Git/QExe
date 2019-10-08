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
    explicit QExeSectionManager(QObject *parent = nullptr);
    void read(QIODevice &src, quint32 sectionCount);
    void write(QIODevice &dst, quint32 fileAlign);
};

#endif // QEXESECTIONMANAGER_H
