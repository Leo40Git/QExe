#include "qexesection.h"

QLatin1String QExeSection::name() const
{
    return QLatin1String(nameBytes);
}

void QExeSection::setName(const QLatin1String &name)
{
    QLatin1String nameT = name;
    if (nameT.size() > 8)
        nameT.truncate(8);
    nameBytes = QByteArray(nameT.data());
}

QExeSection::QExeSection(QObject *parent) : QObject(parent)
{
}
