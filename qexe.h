#ifndef QEXE_H
#define QEXE_H

#include <QObject>
#include <QIODevice>

#include "QExe_global.h"
#include "qexeerrorinfo.h"
#include "qexedosstub.h"
#include "qexecoffheader.h"
#include "qexeoptionalheader.h"
#include "qexesectionmanager.h"
#include "qexersrcmanager.h"

class QEXE_EXPORT QExe : QObject
{
    Q_OBJECT
public:
    template<typename V>
    static V alignForward(V val, V align) {
        V mod = val % align;
        if (mod != 0)
            val += align - mod;
        return val;
    }
    static bool dataDirSectionName(QExeOptionalHeader::DataDirectories dataDir, QLatin1String *secName);
    explicit QExe(QObject *parent = nullptr);
    void reset();
    bool read(QIODevice &src, QExeErrorInfo *errinfo = nullptr);
    bool write(QIODevice &dst, QExeErrorInfo *errinfo = nullptr);
    QSharedPointer<QExeDOSStub> dosStub() const;
    QSharedPointer<QExeCOFFHeader> coffHeader() const;
    QSharedPointer<QExeOptionalHeader> optionalHeader() const;
    QSharedPointer<QExeSectionManager> sectionManager() const;
    QSharedPointer<QExeRsrcManager> rsrcManager() const;
    QSharedPointer<QExeRsrcManager> createRsrcManager(QExeErrorInfo *errinfo = nullptr);
    bool removeRsrcManager(bool keepRsrcSection = true);
    bool autoCreateRsrcManager() const;
    void setAutoCreateRsrcManager(bool autoCreateRsrcManager);
    bool autoAddFillerSections() const;
    void setAutoAddFillerSections(bool autoAddFillerSections);

private:
    friend class QExeCOFFHeader;
    friend class QExeOptionalHeader;
    friend class QExeSectionManager;
    friend class QExeRsrcManager;

    void updateHeaderSizes();
    bool updateComponents(quint32 *fileSize, QExeErrorInfo *error);
    bool m_autoCreateRsrcManager;
    bool m_autoAddFillerSections;
    QSharedPointer<QExeDOSStub> m_dosStub;
    QSharedPointer<QExeCOFFHeader> m_coffHead;
    QSharedPointer<QExeOptionalHeader> m_optHead;
    QSharedPointer<QExeSectionManager> m_secMgr;
    QSharedPointer<QExeRsrcManager> m_rsrcMgr;
    bool isFillerSection(QExeSectionPtr sec);
    QExeSectionPtr createFillerSection(int num, quint32 addr, quint32 size);
    void addFillerSections();
    void removeFillerSections();
};

#endif // QEXE_H
