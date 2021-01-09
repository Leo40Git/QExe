#include "qexersrcentry.h"

QExeRsrcEntry::Type QExeRsrcEntry::type() const
{
    return m_type;
}

std::list<QExeRsrcEntryPtr> QExeRsrcEntry::children() const
{
    return m_children;
}

bool QExeRsrcEntry::addChild(QExeRsrcEntryPtr child)
{
    if (m_type != Directory || child.isNull())
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
    m_children.push_front(child);
    child->m_parent = QExeRsrcEntryPtr(this);
    return true;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChild(QExeRsrcEntry::Type type, const QString &name)
{
    if (m_type != Directory)
        return nullptr;
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(type));
    child->name = name;
    if (!addChild(child))
        return nullptr;
    return child;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChild(QExeRsrcEntry::Type type, const quint32 id)
{
    if (m_type != Directory)
        return nullptr;
    QExeRsrcEntryPtr child = QExeRsrcEntryPtr(new QExeRsrcEntry(type));
    child->id = id;
    if (!addChild(child))
        return nullptr;
    return child;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChildIfAbsent(QExeRsrcEntry::Type type, const QString &name)
{
    QExeRsrcEntryPtr ret = createChild(type, name);
    if (ret.isNull()) {
        ret = child(name);
        if (ret->type() != type)
            ret = nullptr;
    }
    return ret;
}

QExeRsrcEntryPtr QExeRsrcEntry::createChildIfAbsent(QExeRsrcEntry::Type type, const quint32 id)
{
    QExeRsrcEntryPtr ret = createChild(type, id);
    if (ret.isNull()) {
        ret = child(id);
        if (ret->type() != type)
            ret = nullptr;
    }
    return ret;
}

QExeRsrcEntryPtr QExeRsrcEntry::child(const QString &name) const
{
    QExeRsrcEntryPtr entry;
    foreach (entry, m_children) {
        if (name.compare(entry->name) == 0)
            return entry;
    }
    return nullptr;
}

QExeRsrcEntryPtr QExeRsrcEntry::child(const quint32 id) const
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
    m_children.remove(entry);
    return entry;
}

QExeRsrcEntryPtr QExeRsrcEntry::removeChild(const quint32 id)
{
    QExeRsrcEntryPtr entry = child(id);
    if (entry.isNull())
        return entry;
    m_children.remove(entry);
    return entry;
}

std::list<QExeRsrcEntryPtr> QExeRsrcEntry::removeAllChildren()
{
    std::list<QExeRsrcEntryPtr> ret = m_children;
    m_children.clear();
    return ret;
}

QList<QExeRsrcEntryPtr> QExeRsrcEntry::fromPath(const QString &path) const
{
    if (m_type != Directory)
        return QList<QExeRsrcEntryPtr>();
    QString pathT = path.trimmed();
    if (pathT.isEmpty())
        return QList<QExeRsrcEntryPtr>();
    QStringList parts = path.split("/");
    if (parts.size() < 1)
        return QList<QExeRsrcEntryPtr>();
    int partI = 0;
    QExeRsrcEntryConstPtr entry = QExeRsrcEntryConstPtr(this);
    QList<QExeRsrcEntryPtr> results;
    QExeRsrcEntryPtr child;
    while (partI < parts.size()) {
        if (entry.isNull())
            return QList<QExeRsrcEntryPtr>();
        if (entry->type() != Directory)
            return QList<QExeRsrcEntryPtr>();
        QString part = parts[partI++];
        if (part.compare("***") == 0) {
            // wildcard: all
            if (partI != parts.size() - 1)
                return QList<QExeRsrcEntryPtr>();
            foreach (child, entry->children()) {
                results += child;
            }
            return results;
        } else if (part.compare("**") == 0) {
            // wildcard: ID
            if (partI != parts.size() - 1)
                return QList<QExeRsrcEntryPtr>();
            foreach (child, entry->children()) {
                if (child->name.isEmpty())
                    results += child;
            }
            return results;
        } else if (part.compare("*") == 0) {
            // wildcard: name
            if (partI != parts.size() - 1)
                return QList<QExeRsrcEntryPtr>();
            foreach (child, entry->children()) {
                if (!child->name.isEmpty())
                    results += child;
            }
            return results;
        } else {
            if (part.startsWith("*")) {
                // ID
                quint32 id = part.midRef(1).toUInt();
                foreach (child, entry->children()) {
                    if (id == child->id) {
                        results.clear();
                        results += child;
                        entry = child;
                    }
                }
            } else {
                // name
                foreach (child, entry->children()) {
                    if (part.compare(child->name) == 0) {
                        results.clear();
                        results += child;
                        entry = child;
                    }
                }
            }
        }
    }
    return results;
}

QExeRsrcEntry::QExeRsrcEntry(Type type, QObject *parent) : QObject(parent)
{
    m_type = type;
}
