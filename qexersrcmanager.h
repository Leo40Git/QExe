#ifndef QEXERSRCMANAGER_H
#define QEXERSRCMANAGER_H

#include <QObject>

#include "QExe_global.h"
#include "qexesection.h"
#include "qexeerrorinfo.h"
#include "qexersrcentry.h"

class QExe;
class QExeOptionalHeader;
class QExeSectionManager;
class QBuffer;

class QEXE_EXPORT QExeRsrcManager : public QObject
{
    Q_OBJECT
public:
    quint32 headerSize() const;
    QExeRsrcEntryPtr root() const;
    QList<QExeRsrcEntryPtr> entriesFromPath(const QString &path) const;
private:
    friend class QExe;
    friend class QExeOptionalHeader;
    friend class QExeSectionManager;
    friend class QExeRsrcEntry;

    explicit QExeRsrcManager(QExe *exeDat, QObject *parent = nullptr);
    bool read(QExeSectionPtr sec, QExeErrorInfo *errinfo = nullptr);
    void toSection();
    bool readDirectory(QBuffer &src, QExeRsrcEntryPtr dir, quint32 offset);
    bool readEntry(QBuffer &src, QExeRsrcEntryPtr dir, quint32 offset);
    class SectionSizes;
    SectionSizes calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr = nullptr);
    class ReferenceMemory;
    void writeDirectory(QBuffer &dst, QExeRsrcEntryPtr dir, ReferenceMemory &refMem);
    void writeEntries(QBuffer &dst, QLinkedList<QExeRsrcEntryPtr> entries, QLinkedList<QExeRsrcEntryPtr> &subdirs, ReferenceMemory &refMem);
    void writeReferences(QBuffer &dst, SectionSizes sizes, ReferenceMemory &refMem, quint32 offset);
    QExe *exeDat;
    QExeRsrcEntryPtr m_root;
};

#endif // QEXERSRCMANAGER_H
