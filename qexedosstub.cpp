#include "qexedosstub.h"

quint32 QExeDOSStub::size() const
{
    return static_cast<quint32>(data.size());
}

QExeDOSStub::QExeDOSStub(QExe *exeDat, QObject *parent) : QObject(parent)
{
    m_exeDat = exeDat;

    data = QByteArray(0x40, 0);
    // generate a minimal (but functioning) DOS stub header
    data[0] = 0x4D;
    data[1] = 0x5A;
    data[2] = 0x40;
    data[4] = 0x01;
    data[8] = 0x06;
    data[0x3C] = 0x40;
}
