#ifndef QEXERSRCMANAGER_H
#define QEXERSRCMANAGER_H

#include <QObject>

#include "QExe_global.h"
#include "qexesection.h"
#include "qexeerrorinfo.h"
#include "qexersrcentry.h"

class QBuffer;
class QExe;

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

    static void shiftOffsets(QExeSectionPtr rsrcSec, const qint64 shift);

    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, QExeSectionPtr sec);
    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars);
    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, quint32 size, QExeSection::Characteristics chars);
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

    static void shiftDirectory(QBuffer &buf, QDataStream &ds, const qint64 shift);
};

#endif // QEXERSRCMANAGER_H
