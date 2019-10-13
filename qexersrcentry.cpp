#include "qexersrcentry.h"

QExeRsrcEntry::Type QExeRsrcEntry::type() const
{
    return m_type;
}

QLinkedList<QExeRsrcEntryPtr> QExeRsrcEntry::children()
{
    return m_children;
}

bool QExeRsrcEntry::addChild(QExeRsrcEntryPtr child)
{
    if (child.isNull())
        return false;
    QExeRsrcEntryPtr entry;
    QString childName = child->name;
    if (childName.isEmpty()) {
        // check for conflicting IDs
        foreach (entry, m_children) {
            if (child->id == entry->id)
                return false;
        }
    } else {
        // check for conflicting name
        foreach (entry, m_children) {
            if (childName.compare(entry->name) == 0)
                return false;
        }
    }
    m_children += child;
    return true;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChild(QExeRsrcEntry::Type type, const QString &name)
{
    if (m_type != Directory)
        return nullptr;
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(type));
    child->name = name;
    m_children += child;
    return child;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChild(QExeRsrcEntry::Type type, quint32 id)
{
    if (m_type != Directory)
        return nullptr;
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(type));
    child->id = id;
    m_children += child;
    return child;
}

QExeRsrcEntryPtr QExeRsrcEntry::child(const QString &name)
{
    QExeRsrcEntryPtr entry;
    foreach (entry, m_children) {
        if (name.compare(entry->name) == 0)
            return entry;
    }
    return nullptr;
}

QExeRsrcEntryPtr QExeRsrcEntry::child(quint32 id)
{
    QExeRsrcEntryPtr entry;
    foreach (entry, m_children) {
        if (id == entry->id)
            return entry;
    }
    return nullptr;
}

QExeRsrcEntryPtr QExeRsrcEntry::removeChild(const QString &name)
{
    QExeRsrcEntryPtr entry = child(name);
    if (entry.isNull())
        return entry;
    m_children.removeOne(entry);
    return entry;
}

QExeRsrcEntryPtr QExeRsrcEntry::removeChild(quint32 id)
{
    QExeRsrcEntryPtr entry = child(id);
    if (entry.isNull())
        return entry;
    m_children.removeOne(entry);
    return entry;
}

void QExeRsrcEntry::removeAllChildren()
{
    m_children.clear();
}

QExeRsrcEntry::QExeRsrcEntry(Type type, QObject *parent) : QObject(parent)
{
    m_type = type;
}
