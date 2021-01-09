#ifndef QEXERSRCMANAGER_H
#define QEXERSRCMANAGER_H

#include <QObject>

#include "QExe_global.h"
#include "qexesection.h"
#include "qexeerrorinfo.h"
#include "qexersrcentry.h"

class QBuffer;

class QEXE_EXPORT QExeRsrcManager : public QObject
{
    Q_OBJECT
public:
    explicit QExeRsrcManager(QObject *parent = nullptr);
    quint32 headerSize() const;

    bool read(QExeSectionPtr sec, QExeErrorInfo *errinfo = nullptr);
    QExeSectionPtr toSection(quint32 sectionAlign);

    QExeRsrcEntryPtr root() const;
    QList<QExeRsrcEntryPtr> entriesFromPath(const QString &path) const;
private:
    bool readDirectory(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset);
    bool readEntry(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset);
    class SectionSizes;
    SectionSizes calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr = nullptr);
    class SymbolTable;
    void writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, std::list<QExeRsrcEntryPtr> &subdirsName, std::list<QExeRsrcEntryPtr> &subdirsID, SymbolTable &symTbl);
    void writeEntries(QBuffer &dst, QDataStream &ds, std::list<QExeRsrcEntryPtr> entries, std::list<QExeRsrcEntryPtr> &subdirsName, std::list<QExeRsrcEntryPtr> &subdirsID, SymbolTable &symTbl);
    void writeSymbols(QBuffer &dst, QDataStream &ds, SectionSizes sizes, SymbolTable &symTbl, quint32 offset);
    QExeRsrcEntryPtr m_root;
};

#endif // QEXERSRCMANAGER_H
