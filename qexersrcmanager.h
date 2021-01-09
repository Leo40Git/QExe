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

    bool read(QExeSectionPtr sec, QExeErrorInfo *errinfo = nullptr);
    QExeSectionPtr toSection(quint32 sectionAlign);
    bool toSection(QExe &exeDat);

    QExeRsrcEntryPtr root() const;

    static void correctOffsets(QExeSectionPtr rsrcSec, const qint64 shift);

    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, QExeSectionPtr sec);
    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars);
    static bool addBeforeRsrcSection(QSharedPointer<QExeSectionManager> secMgr, const QLatin1String &name, quint32 size, QExeSection::Characteristics chars);
private:
    bool readDirectory(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr dir, quint32 offset);
    bool readEntry(QBuffer &src, QDataStream &ds, QExeRsrcEntryPtr entry, quint32 offset);
    class SectionSizes;
    SectionSizes calculateSectionSizes(QExeRsrcEntryPtr root, QStringList *allocStr = nullptr);
    class SubdirStorage;
    class SymbolTable;
    QExeRsrcEntryPtr m_root;
    void writeDirectory(QBuffer &dst, QDataStream &ds, QExeRsrcEntryPtr dir, SubdirStorage &subdirs, SymbolTable &symTbl);
    void writeEntries(QBuffer &dst, QDataStream &ds, std::list<QExeRsrcEntryPtr> &entries, SymbolTable &symTbl);
    void writeSymbols(QBuffer &dst, QDataStream &ds, const QExeRsrcManager::SectionSizes &sizes, QExeRsrcManager::SymbolTable &symTbl);

    static void shiftDirectory(QBuffer &buf, QDataStream &ds, const qint64 shift, const quint32 ptr);
};

#endif // QEXERSRCMANAGER_H
