#ifndef QEXESECTIONMANAGER_H
#define QEXESECTIONMANAGER_H

#include <QObject>
#include <QBuffer>
#include <QVector>
#include <QSharedPointer>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexesection.h"

class QExe;
class QExeDOSStub;
class QExeCOFFHeader;
class QExeOptionalHeader;

class QEXE_EXPORT QExeSectionManager : public QObject
{
    Q_OBJECT
public:
    quint32 headerSize() const;
    int sectionCount() const;
    QExeSectionPtr sectionAt(int index) const;
    int sectionIndexByName(const QLatin1String &name) const;
    QExeSectionPtr sectionWithName(const QLatin1String &name) const;
    bool containsSection(QExeSectionPtr sec) const;
    QVector<bool> containsSections(QVector<QExeSectionPtr> secs) const;
    bool addSection(QExeSectionPtr newSec);
    QVector<bool> addSections(QVector<QExeSectionPtr> newSecs);
    bool removeSection(QExeSectionPtr sec);
    QVector<bool> removeSections(QVector<QExeSectionPtr> secs);
    QExeSectionPtr removeSection(int index);
    QVector<QExeSectionPtr> removeSections(QVector<int> indexes);
    QExeSectionPtr removeSection(const QLatin1String &name);
    QVector<QExeSectionPtr> removeSections(QVector<QLatin1String> names);
    QExeSectionPtr createSection(const QLatin1String &name, quint32 size);
    QBuffer *setupRVAPoint(quint32 rva, QIODevice::OpenMode mode) const;
private:
    friend class QExe;

    explicit QExeSectionManager(QExe *exeDat, QObject *parent = nullptr);
    QExe *exeDat;
    QVector<QExeSectionPtr> sections;
    bool read(QIODevice &src, QDataStream &ds, QExeErrorInfo *errinfo);
    bool write(QIODevice &dst, QDataStream &ds, QExeErrorInfo *errinfo);
    bool test(bool justOrderAndOverlap, quint32 *fileSize = nullptr, QExeErrorInfo *errinfo = nullptr);
    void positionSection(QExeSectionPtr newSec, quint32 i, quint32 sectionAlign);
    int rsrcSectionIndex();
};

#endif // QEXESECTIONMANAGER_H
