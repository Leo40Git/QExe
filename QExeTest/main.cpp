#include <QFile>
#include <QDebug>
#include <QMetaEnum>

#include "qexe.h"

int main(int argc, char *argv[])
{
    (void)argc, (void)argv;

    QExe exeDat;

    QString dataPath("C:/QExeTest");
    QFile exeFile(dataPath + "/Doukutsu.exe");
    exeFile.open(QFile::ReadOnly);
    QExeErrorInfo errinfo;
    if (exeDat.read(exeFile, &errinfo)) {
        exeFile.close();
        qDebug().noquote().nospace() << "Error while reading EXE file \"" << dataPath << "/" << exeFile.fileName() << "\":" << endl;
        qDebug().noquote().nospace() << errinfo.errorID << endl;
        qDebug().noquote().nospace() << errinfo.details << endl;
        return 0;
    }

    QByteArray exeDatNew = exeDat.toBytes();
    exeFile.seek(0);
    QByteArray exeDatOld = exeFile.read(exeDatNew.size());
    exeFile.close();
    QFile outOld(dataPath + "/out_old.header");
    outOld.open(QFile::WriteOnly);
    outOld.write(exeDatOld);
    outOld.close();
    QFile outNew(dataPath + "/out_new.header");
    outNew.open(QFile::WriteOnly);
    outNew.write(exeDatNew);
    outNew.close();
}
