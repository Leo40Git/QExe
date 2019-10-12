#ifndef QEXESECTIONMANAGER_H
#define QEXESECTIONMANAGER_H

#include <QObject>
#include <QIODevice>
#include <QVector>
#include <QSharedPointer>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexesection.h"

class QExe;
class QExeDOSStub;
class QExeCOFFHeader;
class QExeOptionalHeader;

typedef QSharedPointer<QExeSection> QExeSectionPtr;

class QEXE_EXPORT QExeSectionManager : public QObject
{
    Q_OBJECT
    friend class QExe;
    friend class QExeDOSStub;
    friend class QExeCOFFHeader;
    friend class QExeOptionalHeader;
public:
    quint32 headerSize();
    int sectionCount();
    QExeSectionPtr sectionAt(int index);
    int sectionIndexByName(const QLatin1String &name);
    QExeSectionPtr sectionWithName(const QLatin1String &name);
    bool addSection(QExeSectionPtr newSec, QExeErrorInfo *errinfo = nullptr);
    QExeSectionPtr removeSection(int index);
    QExeSectionPtr removeSection(const QLatin1String &name);
    QExeSectionPtr createSection(const QLatin1String &name, quint32 size);
private:
    explicit QExeSectionManager(QExe *exeDat, QObject *parent = nullptr);
    QExe *exeDat;
    QVector<QExeSectionPtr> sections;
    void read(QIODevice &src);
    void write(QIODevice &dst);
    bool test(QExeErrorInfo *errinfo);
};

#endif // QEXESECTIONMANAGER_H
