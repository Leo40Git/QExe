#include <QFile>
#include <QDebug>
#include <QMetaEnum>

#include "qexe.h"

#define OUT qInfo().noquote().nospace()
#define HEX(n) "0x" << QString::number(n, 16).toUpper()

int main(int argc, char *argv[])
{
    (void)argc, (void)argv;

    QExe exeDat;

    QString testPath("CaveStoryEN");
    QFile exeFile(testPath + "/Doukutsu.exe");
    if (!exeFile.exists()) {
        OUT << "Could not find EXE file \"" << exeFile.fileName() << "\":";
        return 1;
    }
    exeFile.open(QFile::ReadOnly);
    QExeErrorInfo errinfo;
    if (exeDat.read(exeFile, &errinfo)) {
        exeFile.close();
        OUT << "Error while reading EXE file \"" << exeFile.fileName() << "\":";
        OUT << "ID: " << errinfo.errorID;
        OUT << "Details: " << errinfo.details;
        return 1;
    }

    QExeCOFFHeader &coffHead = exeDat.coffHead();
    OUT << "Printing COFF header properties";
    OUT << "Machine type: " << coffHead.machineType;
    OUT << "Timestamp: " << HEX(coffHead.timestamp);
    OUT << "Symbol table pointer: " << HEX(coffHead.symTblPtr);
    OUT << "Symbol table count: " << HEX(coffHead.symTblCount);
    OUT << "Characteristics: " << coffHead.characteristics;
    QExeOptHeader &optHead = exeDat.optHead();
    OUT << "Printing optional header properties";
    OUT << "Linker version: " << HEX(optHead.linkerVerMajor) << "/" << HEX(optHead.linkerVerMinor);
    OUT << "Size of code: " << HEX(optHead.codeSize);
    OUT << "Size of initialized data: " << HEX(optHead.initializedDataSize);
    OUT << "Size of uninitialized data: " << HEX(optHead.uninitializedDataSize);
    OUT << "Address of entry point: " << HEX(optHead.entryPointAddr);
    OUT << "Base of code address: " << HEX(optHead.codeBaseAddr);
    OUT << "Base of data address: " << HEX(optHead.dataBaseAddr);

    QByteArray exeDatNew = exeDat.toBytes();
    exeFile.seek(0);
    QByteArray exeDatOld = exeFile.read(exeDatNew.size());
    exeFile.close();
    QFile outOld(testPath + "/out_old.header");
    outOld.open(QFile::WriteOnly);
    outOld.write(exeDatOld);
    outOld.close();
    QFile outNew(testPath + "/out_new.header");
    outNew.open(QFile::WriteOnly);
    outNew.write(exeDatNew);
    outNew.close();

    return 0;
}
