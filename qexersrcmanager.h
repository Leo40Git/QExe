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

    explicit QExeRsrcManager(QExe *exeDat, QObject *parent = nullptr);
    bool read(QExeSectionPtr sec, QExeErrorInfo *errinfo = nullptr);
    void toSection();
    bool readDirectory(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset);
    bool readEntry(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset);
    class SectionSizes;
    SectionSizes calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr = nullptr);
    class SymbolTable;
    void writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, QLinkedList<QExeRsrcEntryPtr> &subdirsName, QLinkedList<QExeRsrcEntryPtr> &subdirsID, SymbolTable &symTbl);
    void writeEntries(QBuffer &dst, QDataStream &ds, QLinkedList<QExeRsrcEntryPtr> entries, QLinkedList<QExeRsrcEntryPtr> &subdirsName, QLinkedList<QExeRsrcEntryPtr> &subdirsID, SymbolTable &symTbl);
    void writeSymbols(QBuffer &dst, QDataStream &ds, SectionSizes sizes, SymbolTable &symTbl, quint32 offset);
    QExe *exeDat;
    QExeRsrcEntryPtr m_root;
};

#endif // QEXERSRCMANAGER_H
