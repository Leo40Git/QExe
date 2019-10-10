#include "qexesection.h"

QLatin1String QExeSection::name() const
{
    return QLatin1String(nameBytes);
}

void QExeSection::setName(const QLatin1String &name)
{
    nameBytes = QByteArray(name.data());
    nameBytes.resize(8);
}

QExeSection::QExeSection(QObject *parent) : QObject(parent)
{
}
