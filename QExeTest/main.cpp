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
    if (!exeDat.read(exeFile, &errinfo)) {
        exeFile.close();
        OUT << "Error while reading EXE file \"" << exeFile.fileName() << "\":";
        OUT << "ID: " << errinfo.errorID;
        OUT << "Details: " << errinfo.details;
        return 1;
    }

    QSharedPointer<QExeCOFFHeader> coffHead = exeDat.coffHead();
    OUT << "Printing COFF header properties";
    OUT << "Machine type: " << coffHead->machineType;
    OUT << "Timestamp: " << HEX(coffHead->timestamp);
    OUT << "Symbol table pointer: " << HEX(coffHead->symTblPtr);
    OUT << "Symbol table count: " << HEX(coffHead->symTblCount);
    OUT << "Characteristics: " << coffHead->characteristics;
    QSharedPointer<QExeOptHeader> optHead = exeDat.optHead();
    OUT << "Printing optional header properties";
    OUT << "Linker version: " << HEX(optHead->linkerVerMajor) << "/" << HEX(optHead->linkerVerMinor);
    OUT << "Address of entry point: " << HEX(optHead->entryPointAddr);
    OUT << "Base of code address: " << HEX(optHead->codeBaseAddr);
    OUT << "Base of data address: " << HEX(optHead->dataBaseAddr);
    OUT << "Image base: " << HEX(optHead->imageBase);
    OUT << "Section alignment: " << HEX(optHead->sectionAlign);
    OUT << "File alignment: " << HEX(optHead->fileAlign);
    OUT << "Minumum OS version: " << HEX(optHead->minOSVerMajor) << "/" << HEX(optHead->minOSVerMinor);
    OUT << "Image version: " << HEX(optHead->imageVerMajor) << "/" << HEX(optHead->imageVerMinor);
    OUT << "Subsystem version: " << HEX(optHead->subsysVerMajor) << "/" << HEX(optHead->subsysVerMinor);
    OUT << "\"Win32VersionValue\" (reserved, must be 0): " << HEX(optHead->win32VerValue);
    OUT << "Checksum: " << HEX(optHead->checksum);
    OUT << "Subsystem: " << optHead->subsystem;
    OUT << "DLL characteristics: " << optHead->dllCharacteristics;
    OUT << "Stack reserve size: " << HEX(optHead->stackReserveSize);
    OUT << "Stack commit size: " << HEX(optHead->stackCommitSize);
    OUT << "Heap reserve size: " << HEX(optHead->heapReserveSize);
    OUT << "Heap commit size: " << HEX(optHead->heapCommitSize);
    OUT << "\"LoaderFlags\" (reserved, must be 0): " << HEX(optHead->loaderFlags);
    QSharedPointer<QList<QPair<quint32, quint32>>> dataDirectories = optHead->dataDirectories();
    if (dataDirectories.isNull()) {
        OUT << "Image data directories: null (0 total)";
    } else {
        OUT << "Image data directories: " << dataDirectories->size() << " total";
        QPair<quint32, quint32> dir;
        foreach (dir, *dataDirectories) {
            OUT << "  RVA: " << HEX(dir.first) << ", size: " << HEX(dir.second);
        }
    }

    QByteArray exeDatNew = exeDat.toBytes();
    exeFile.seek(0);
    QByteArray exeDatOld = exeFile.read(exeDatNew.size());
    exeFile.close();
    QFile outOld(testPath + "/out_old.header");
    outOld.open(QFile::WriteOnly);
    outOld.write(exeDatOld);
    outOld.close();
    OUT << "Wrote old header to \"" << outOld.fileName() << "\"";
    QFile outNew(testPath + "/out_new.header");
    outNew.open(QFile::WriteOnly);
    outNew.write(exeDatNew);
    outNew.close();
    OUT << "Wrote new header to \"" << outNew.fileName() << "\"";

    return 0;
}
