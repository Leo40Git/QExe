#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMetaEnum>

#include "qexe.h"

#include <iostream>

#define OUT qInfo().noquote().nospace()
#define HEX(n) "0x" << QString::number(n, 16).toUpper()

void rsrcPrintDirectory(const QString &indent, QExeRsrcEntryPtr dir) {
    OUT << indent << " \"Characteristics\" (reserved, must be 0): " << dir->directoryMeta.characteristics;
    OUT << indent << " Timestamp: " << dir->directoryMeta.timestamp;
    OUT << indent << " Version: " << dir->directoryMeta.version.first << "." << dir->directoryMeta.version.second;
    QLinkedList<QExeRsrcEntryPtr> children = dir->children();
    OUT << indent << " " << children.size() << " entries:";
    QExeRsrcEntryPtr child;
    foreach (child, children) {
        OUT << indent << "> Type: " << child->type();
        if (child->name.isEmpty())
            OUT << indent << "> ID: " << HEX(child->id);
        else
            OUT << indent << "> Name: " << child->name;
        if (child->type() == QExeRsrcEntry::Directory) {
            rsrcPrintDirectory(indent + ">", child);
            continue;
        }
        OUT << indent << "> Data size: " << HEX(child->data.size());
        OUT << indent << "> Codepage: " << HEX(child->dataMeta.codepage);
        OUT << indent << "> (reserved, must be 0): " << HEX(child->dataMeta.reserved);
    }
}

void waitForEnter() {
    static std::string line;
    std::getline(std::cin, line);
}

int main(int argc, char *argv[])
{
    (void)argc, (void)argv;

    QExe exeDat;
    exeDat.setAutoCreateRsrcManager(false);

    QDir testDir("CaveStoryEN");
    testDir.makeAbsolute();
    QFile exeFile(QStringLiteral("%1/Doukutsu.exe").arg(testDir.absolutePath()));
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
    OUT << " == Printing COFF header properties == ";
    OUT << " Machine type: " << coffHead->machineType;
    OUT << " Timestamp: " << HEX(coffHead->timestamp);
    OUT << " Symbol table pointer: " << HEX(coffHead->symTblPtr);
    OUT << " Symbol table count: " << HEX(coffHead->symTblCount);
    OUT << " Characteristics: " << coffHead->characteristics;
    QSharedPointer<QExeOptionalHeader> optHead = exeDat.optionalHeader();
    OUT << " == Printing optional header properties == ";
    OUT << " Linker version: " << optHead->linkerVer.first << "." << optHead->linkerVer.second;
    OUT << " Address of entry point: " << HEX(optHead->entryPointAddr);
    OUT << " Base of code address: " << HEX(optHead->codeBaseAddr);
    OUT << " Base of data address: " << HEX(optHead->dataBaseAddr);
    OUT << " Image base: " << HEX(optHead->imageBase);
    OUT << " Section alignment: " << HEX(optHead->sectionAlign);
    OUT << " File alignment: " << HEX(optHead->fileAlign);
    OUT << " Minumum OS version: " << optHead->minOSVer.first << "." << optHead->minOSVer.second;
    OUT << " Image version: " << optHead->imageVer.first << "." << optHead->imageVer.second;
    OUT << " Subsystem version: " << optHead->subsysVer.first << "." << optHead->subsysVer.second;
    OUT << " \"Win32VersionValue\" (reserved, must be 0): " << HEX(optHead->win32VerValue);
    OUT << " Checksum: " << HEX(optHead->checksum);
    OUT << " Subsystem: " << optHead->subsystem;
    OUT << " DLL characteristics: " << optHead->dllCharacteristics;
    OUT << " Stack reserve size: " << HEX(optHead->stackReserveSize);
    OUT << " Stack commit size: " << HEX(optHead->stackCommitSize);
    OUT << " Heap reserve size: " << HEX(optHead->heapReserveSize);
    OUT << " Heap commit size: " << HEX(optHead->heapCommitSize);
    OUT << " \"LoaderFlags\" (reserved, must be 0): " << HEX(optHead->loaderFlags);
    int dirCount;
    OUT << "Image data directories: (" << (dirCount = optHead->dataDirectories.size()) << " total)";
    QMetaEnum dirMeta = QMetaEnum::fromType<QExeOptionalHeader::DataDirectories>();
    DataDirectoryPtr dir;
    for (int i = 0; i < dirCount; i++) {
        dir = optHead->dataDirectories[i];
        OUT << "  " << dirMeta.valueToKey(i) << " - Address: " << HEX(dir->first) << ", size: " << HEX(dir->second);
    }
    QSharedPointer<QExeSectionManager> secMgr = exeDat.sectionManager();
    int secCount;
    OUT << " == Printing sections == (" << (secCount = secMgr->sectionCount()) << " total)";
    QExeSectionPtr section;
    for (int i = 0; i < secCount; i++) {
        section = secMgr->sectionAt(i);
        OUT << " - \"" << section->name() << "\"";
        OUT << " Linearized: " << section->linearize;
        OUT << " Virtual size: " << HEX(section->virtualSize);
        OUT << " Virtual address: " << HEX(section->virtualAddr);
        OUT << " Raw data size: " << HEX(section->rawData.size());
        OUT << " Relocations pointer: " << HEX(section->relocsPtr);
        OUT << " Line-numbers poitner: " << HEX(section->linenumsPtr);
        OUT << " Relocations count: " << HEX(section->relocsCount);
        OUT << " Line-numbers count: " << HEX(section->linenumsCount);
        OUT << " Characteristics: " << section->characteristics;
    }

    OUT << "Press <RETURN> to continue.";
    waitForEnter();

    // dump pure .rsrc section
    QFile rsrcSecFile(QStringLiteral("%1/rsrc.sec").arg(testDir.absolutePath()));
    {
        QExeSectionPtr rsrcSec = secMgr->sectionAt(secMgr->rsrcSectionIndex());
        rsrcSecFile.open(QFile::WriteOnly);
        rsrcSecFile.write(rsrcSec->rawData);
        rsrcSecFile.close();
        rsrcSec = nullptr;
        OUT << "Dumped original .rsrc section to \"" << rsrcSecFile.fileName() << "\"";
    }

    QSharedPointer<QExeRsrcManager> rsrcMgr = exeDat.rsrcManager();
    if (!rsrcMgr) {
        rsrcMgr = exeDat.createRsrcManager(&errinfo);
        if (!rsrcMgr) {
            OUT << "Error while reading \".rsrc\" section:";
            OUT << "ID: " << errinfo.errorID;
            OUT << "Details: " << errinfo.details;
            return 1;
        }
    }

    /*
    if (rsrcMgr) {
        OUT << " == Printing .rsrc section contents (root directory) == ";
        rsrcPrintDirectory(QStringLiteral(""), rsrcMgr->root());
    }
    */

    /*
    OUT << " == Dumping Dialog/DLG_ABOUT to a file ==";
    QExeRsrcEntryPtr dialogDir = rsrcMgr->root()->child(QExeRsrcEntry::Dialog);
    QExeRsrcEntryPtr dlgAbout = dialogDir->child("DLG_ABOUT");
    if (!dlgAbout)
        OUT << "DLG_ABOUT don't bloody exist";
    else if (dlgAbout->type() != QExeRsrcEntry::Directory)
        OUT << "DLG_ABOUT ain't a fookin directory";
    else {
        QLinkedList<QExeRsrcEntryPtr> dlgAboutC = dlgAbout->children();
        if (dlgAboutC.size() != 1)
            OUT << "DLG_ABOUT don't have only 1 child, the git";
        else {
            QExeRsrcEntryPtr dlgAboutData = dlgAboutC.first();
            QFile outAboutData(QStringLiteral("%1/DLG_ABOUT.bin").arg(testDir.absolutePath()));
            outAboutData.open(QFile::WriteOnly);
            outAboutData.write(dlgAboutData->data);
            outAboutData.close();
            OUT << "Threw DLG_ABOUT into \"" << outAboutData.fileName() << "\"";
        }
    }
    */

    QFile outNew(QStringLiteral("%1/Doukutsu.out.exe").arg(testDir.absolutePath()));
    outNew.open(QFile::WriteOnly);
    if (!exeDat.write(outNew, &errinfo)) {
        OUT << "Error while writing EXE file \"" << outNew.fileName() << "\":";
        OUT << "ID: " << errinfo.errorID;
        OUT << "Details: " << errinfo.details;
        return 1;
    }
    outNew.close();
    OUT << "Wrote new EXE to \"" << outNew.fileName() << "\"";

    OUT << "Press <RETURN> to continue.";
    waitForEnter();

    /*
    exeDat.removeRsrcManager(true);

    // dump modified .rsrc section
    QFile rsrcSecOutFile(QStringLiteral("%1/rsrc.out.sec").arg(testDir.absolutePath()));
    {
        QExeSectionPtr rsrcSec = secMgr->sectionAt(secMgr->rsrcSectionIndex());
        rsrcSecOutFile.open(QFile::WriteOnly);
        rsrcSecOutFile.write(rsrcSec->rawData);
        rsrcSecOutFile.close();
        rsrcSec = nullptr;
        OUT << "Dumped rebuilt .rsrc section to \"" << rsrcSecOutFile.fileName() << "\"";
    }

    // remove modified .rsrc section and replace with original
    secMgr->removeSection(secMgr->rsrcSectionIndex());
    rsrcSecFile.open(QFile::ReadOnly);
    QByteArray data = rsrcSecFile.readAll();
    rsrcSecFile.close();
    secMgr->createSection(QLatin1String(".rsrc"), data);

    // create rebuilt EXE with original .rsrc section
    QFile outNew2(QStringLiteral("%1/Doukutsu.out2.exe").arg(testDir.absolutePath()));
    outNew2.open(QFile::WriteOnly);
    if (!exeDat.write(outNew2, &errinfo)) {
        OUT << "Error while writing EXE file \"" << outNew.fileName() << "\":";
        OUT << "ID: " << errinfo.errorID;
        OUT << "Details: " << errinfo.details;
        return 1;
    }
    outNew2.close();
    OUT << "Wrote new EXE with original .rsrc section to \"" << outNew2.fileName() << "\"";

    OUT << "Press <RETURN> to continue.";
    waitForEnter();
    */

    return 0;
}
