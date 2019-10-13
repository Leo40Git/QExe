#ifndef QEXERSRCENTRY_H
#define QEXERSRCENTRY_H

#include "QExe_global.h"

class QExeRsrcManager;

#include <QSharedPointer>
#include <QLinkedList>

#include "typedef_version.h"

class QExeRsrcEntry;
typedef QSharedPointer<QExeRsrcEntry> QExeRsrcEntryPtr;

class QEXE_EXPORT QExeRsrcEntry : public QObject
{
    Q_OBJECT
public:
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
    QLinkedList<QExeRsrcEntryPtr> children();
    bool addChild(QExeRsrcEntryPtr child);
    QExeRsrcEntryPtr createChild(Type type, const QString &name);
    QExeRsrcEntryPtr createChild(Type type, quint32 id);
    QExeRsrcEntryPtr child(const QString &name);
    QExeRsrcEntryPtr child(quint32 id);
    QExeRsrcEntryPtr removeChild(const QString &name);
    QExeRsrcEntryPtr removeChild(quint32 id);
    void removeAllChildren();
private:
    friend class QExeRsrcManager;

    explicit QExeRsrcEntry(Type type, QObject *parent = nullptr);
    Type m_type;
    QLinkedList<QExeRsrcEntryPtr> m_children;
};

#endif // QEXERSRCENTRY_H
