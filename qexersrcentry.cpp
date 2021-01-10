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
    m_children.push_back(child);
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

QString QExeRsrcEntry::path() const {
    if (m_parent.isNull())
        return QStringLiteral("/");
    return QString("%1%2%3").arg(
                m_parent->path(),
                name.isEmpty() ? QString("*%1").arg(id) : name,
                m_type == Directory ? "/" : ""
                );
}

QExeRsrcEntry::QExeRsrcEntry(Type type, QObject *parent) : QObject(parent)
{
    m_type = type;
}
