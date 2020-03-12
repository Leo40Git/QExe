#ifndef TYPEDEF_VERSION_H
#define TYPEDEF_VERSION_H

#include <QPair>

// first => major, second => minor
typedef QPair<quint8, quint8> Version8;
typedef QPair<quint16, quint16> Version16;

namespace OSVersion {
    // https://www.gaijin.at/en/infos/windows-version-numbers
    // these can be used for both minOSVer and subsysVer
    const Version16 Windows10 = Version16(10, 0);
    const Version16 WindowsServer19 = Windows10;
    const Version16 WindowsServer16 = Windows10;
    const Version16 Windows81 = Version16(6, 3);
    const Version16 WindowsServer12R2 = Windows81;
    const Version16 Windows8 = Version16(6, 2);
    const Version16 WindowsServer12 = Windows8;
    const Version16 Windows7 = Version16(6, 1);
    const Version16 WindowsServer08R2 = Windows7;
    const Version16 WindowsVista = Version16(6, 0);
    const Version16 WindowsServer08 = WindowsVista;
    const Version16 WindowsXP64 = Version16(5, 2);
    const Version16 WindowsServer03R2 = WindowsXP64;
    const Version16 WindowsServer03 = WindowsXP64;
    const Version16 WindowsXP = Version16(5, 1);
    const Version16 Windows2K = Version16(5, 0);
    const Version16 WindowsNT4 = Version16(4, 0);
    const Version16 WindowsNT351 = Version16(3, 51);
    const Version16 WindowsNT35 = Version16(3, 50);
    const Version16 Windows31 = Version16(3, 10);
    const Version16 WindowsME = Version16(4, 90);
    const Version16 Windows98SE = Version16(4, 10);
    const Version16 Windows98 = Windows98SE;
    const Version16 Windows95 = WindowsNT4;
}

#endif // TYPEDEF_VERSION_H
