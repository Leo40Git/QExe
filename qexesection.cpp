#include "qexesection.h"

QExeSection::QExeSection(QObject *parent) : QObject(parent)
{
    linearize = false;
}

QExeSection::QExeSection(const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars, QObject *parent)
    : QObject(parent)
{
    setName(name);
    rawData = data;
    virtualSize = rawData.size();
    characteristics = chars;
    linearize = false;
}

QExeSection::QExeSection(const QLatin1String &name, quint32 size, QExeSection::Characteristics chars, QObject *parent)
    : QObject(parent)
{
    setName(name);
    virtualSize = size;
    rawData.resize(static_cast<int>(size));
    characteristics = chars;
    linearize = false;
}

QLatin1String QExeSection::name() const
{
    return QLatin1String(nameBytes);
}

void QExeSection::setName(const QLatin1String &name)
{
    nameBytes = QByteArray(name.data());
    nameBytes.resize(8);
}
