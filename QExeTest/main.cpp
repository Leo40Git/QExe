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

    QSharedPointer<QExeCOFFHeader> coffHead = exeDat.coffHeader();
    OUT << "Printing COFF header properties";
    OUT << "Machine type: " << coffHead->machineType;
    OUT << "Timestamp: " << HEX(coffHead->timestamp);
    OUT << "Symbol table pointer: " << HEX(coffHead->symTblPtr);
    OUT << "Symbol table count: " << HEX(coffHead->symTblCount);
    OUT << "Characteristics: " << coffHead->characteristics;
    QSharedPointer<QExeOptionalHeader> optHead = exeDat.optionalHeader();
    OUT << "Printing optional header properties";
    OUT << "Linker version: " << optHead->linkerVer.first << "." << optHead->linkerVer.second;
    OUT << "Address of entry point: " << HEX(optHead->entryPointAddr);
    OUT << "Base of code address: " << HEX(optHead->codeBaseAddr);
    OUT << "Base of data address: " << HEX(optHead->dataBaseAddr);
    OUT << "Image base: " << HEX(optHead->imageBase);
    OUT << "Section alignment: " << HEX(optHead->sectionAlign);
    OUT << "File alignment: " << HEX(optHead->fileAlign);
    OUT << "Minumum OS version: " << optHead->minOSVer.first << "." << optHead->minOSVer.second;
    OUT << "Image version: " << optHead->imageVer.first << "." << optHead->imageVer.second;
    OUT << "Subsystem version: " << optHead->subsysVer.first << "." << optHead->subsysVer.second;
    OUT << "\"Win32VersionValue\" (reserved, must be 0): " << HEX(optHead->win32VerValue);
    OUT << "Checksum: " << HEX(optHead->checksum);
    OUT << "Subsystem: " << optHead->subsystem;
    OUT << "DLL characteristics: " << optHead->dllCharacteristics;
    OUT << "Stack reserve size: " << HEX(optHead->stackReserveSize);
    OUT << "Stack commit size: " << HEX(optHead->stackCommitSize);
    OUT << "Heap reserve size: " << HEX(optHead->heapReserveSize);
    OUT << "Heap commit size: " << HEX(optHead->heapCommitSize);
    OUT << "\"LoaderFlags\" (reserved, must be 0): " << HEX(optHead->loaderFlags);
    int dirCount;
    OUT << "Image data directories: " << (dirCount = optHead->dataDirectories.size()) << " total";
    QMetaEnum dirMeta = QMetaEnum::fromType<QExeOptionalHeader::DataDirectories>();
    DataDirectoryPtr dir;
    for (int i = 0; i < dirCount; i++) {
        dir = optHead->dataDirectories[i];
        OUT << "  " << dirMeta.valueToKey(i) << " - Address: " << HEX(dir->first) << ", size: " << HEX(dir->second);
    }
    QSharedPointer<QExeSectionManager> secMgr = exeDat.sectionManager();
    OUT << "Printing sections (" << secMgr->sections.size() << " total)";
    QSharedPointer<QExeSection> section;
    foreach (section, secMgr->sections) {
        OUT << "\"" << section->name() << "\"";
        OUT << " Virtual size: " << HEX(section->virtualSize);
        OUT << " Virtual address: " << HEX(section->virtualAddr);
        OUT << " Raw data size: " << HEX(section->rawData.size());
        OUT << " Relocations pointer: " << HEX(section->relocsPtr);
        OUT << " Line-numbers poitner: " << HEX(section->linenumsPtr);
        OUT << " Relocations count: " << HEX(section->relocsCount);
        OUT << " Line-numbers count: " << HEX(section->linenumsCount);
        OUT << " Characteristics: " << section->characteristics;
    }

    QFile outNew(testPath + "/Doukutsu.out.exe");
    QByteArray exeDatNew;
    if (!exeDat.toBytes(exeDatNew, &errinfo)) {
        OUT << "Error while writing EXE file \"" << outNew.fileName() << "\":";
        OUT << "ID: " << errinfo.errorID;
        OUT << "Details: " << errinfo.details;
        return 1;
    }
    outNew.open(QFile::WriteOnly);
    outNew.write(exeDatNew);
    outNew.close();
    OUT << "Wrote new EXE to \"" << outNew.fileName() << "\"";

    return 0;
}
