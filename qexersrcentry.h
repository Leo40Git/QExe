#ifndef QEXERSRCENTRY_H
#define QEXERSRCENTRY_H

#include "QExe_global.h"

class QExeRsrcManager;

#include <QSharedPointer>
#include <QLinkedList>

#include "typedef_version.h"

class QExeRsrcEntry;
typedef QSharedPointer<QExeRsrcEntry> QExeRsrcEntryPtr;
typedef QSharedPointer<const QExeRsrcEntry> QExeRsrcEntryConstPtr;

class QEXE_EXPORT QExeRsrcEntry : public QObject
{
    Q_OBJECT
public:
    enum RootDirectory : quint32 {
        Cursor = 0x1,
        Bitmap,
        Icon,
        Menu,
        Dialog,
        StringTable,
        Accelerators = 0x9,
        RCData,
        MessageTable,
        CursrorGroup,
        IconGroup = 0xE,
        VersionInfo = 0x10,
        AniCursor = 0x15,
        AniIcon,
        HTML,
        Manifest
    };
    Q_ENUM(RootDirectory);

    enum Type : bool {
        Data,
        Directory
    };
    Q_ENUM(Type)
    Type type() const;
    QString name;
    quint32 id;
    QByteArray data;
    struct {
        quint32 codepage;
        quint32 reserved;
    } dataMeta;
    struct {
        quint32 characteristics;
        quint32 timestamp;
        Version16 version;
    } directoryMeta;
    QLinkedList<QExeRsrcEntryPtr> children() const;
    bool addChild(QExeRsrcEntryPtr child);
    QExeRsrcEntryPtr createChild(Type type, const QString &name);
    QExeRsrcEntryPtr createChild(Type type, quint32 id);
    QExeRsrcEntryPtr child(const QString &name) const;
    QExeRsrcEntryPtr child(quint32 id) const;
    QExeRsrcEntryPtr removeChild(const QString &name);
    QExeRsrcEntryPtr removeChild(quint32 id);
    QLinkedList<QExeRsrcEntryPtr> removeAllChildren();
    QList<QExeRsrcEntryPtr> fromPath(const QString &path) const;
private:
    friend class QExeRsrcManager;

    explicit QExeRsrcEntry(Type type, QObject *parent = nullptr);
    Type m_type;
    QLinkedList<QExeRsrcEntryPtr> m_children;
};

#endif // QEXERSRCENTRY_H
