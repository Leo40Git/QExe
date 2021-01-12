#include "qexesection.h"

QExeSection::QExeSection(QObject *parent) : QObject(parent)
{
    setName(QLatin1String(""));
    virtualAddr = 0;
    virtualSize = 0;
    rawData.resize(0);
    rawDataPtr = 0;
    characteristics = Characteristics();
    linearize = false;
}

QExeSection::QExeSection(const QLatin1String &name, QByteArray data, QExeSection::Characteristics chars, QObject *parent)
    : QObject(parent)
{
    setName(name);
    virtualAddr = 0;
    virtualSize = rawData.size();
    rawData = data;
    rawDataPtr = 0;
    characteristics = chars;
    linearize = false;
}

QExeSection::QExeSection(const QLatin1String &name, quint32 size, QExeSection::Characteristics chars, QObject *parent)
    : QObject(parent)
{
    setName(name);
    virtualAddr = 0;
    virtualSize = size;
    rawData.resize(static_cast<int>(size));
    rawDataPtr = 0;
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
