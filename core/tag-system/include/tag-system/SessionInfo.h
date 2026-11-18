#pragma once

#include <QObject>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::System {

/// Read-only facts about the running user session, collected once.
///
/// TagSettings (Kullanıcı) and TagAbout (Hakkında) show these instead of
/// hardcoded strings. Values come from the environment, the XDG base directory
/// spec and the build; nothing is invented when a value is missing.
class TAGOS_EXPORT SessionInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString homePath READ homePath CONSTANT)
    Q_PROPERTY(QString shellPath READ shellPath CONSTANT)
    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(QString desktopName READ desktopName CONSTANT)
    Q_PROPERTY(QString sessionType READ sessionType CONSTANT)
    Q_PROPERTY(QString localeName READ localeName CONSTANT)
    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(QString kernelVersion READ kernelVersion CONSTANT)
    Q_PROPERTY(QString architecture READ architecture CONSTANT)
    Q_PROPERTY(QString prettyOsName READ prettyOsName CONSTANT)
    Q_PROPERTY(QString tagosVersion READ tagosVersion CONSTANT)
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(QString totalMemoryText READ totalMemoryText CONSTANT)
    Q_PROPERTY(QString buildType READ buildType CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QStringList standardFolders READ standardFolders CONSTANT)

public:
    explicit SessionInfo(QObject *parent = nullptr);

    QString userName() const { return m_userName; }
    QString homePath() const { return m_homePath; }
    QString shellPath() const { return m_shellPath; }
    QString hostName() const { return m_hostName; }
    QString desktopName() const { return m_desktopName; }
    QString sessionType() const { return m_sessionType; }
    QString localeName() const { return m_localeName; }
    QString osName() const { return m_osName; }
    QString kernelVersion() const { return m_kernelVersion; }
    QString architecture() const { return m_architecture; }
    QString prettyOsName() const { return m_prettyOsName; }
    QString tagosVersion() const;
    QString cpuModel() const { return m_cpuModel; }
    QString totalMemoryText() const { return m_totalMemoryText; }
    QString buildType() const;
    QString qtVersion() const;
    QStringList standardFolders() const { return m_standardFolders; }

private:
    QString m_userName;
    QString m_homePath;
    QString m_shellPath;
    QString m_hostName;
    QString m_desktopName;
    QString m_sessionType;
    QString m_localeName;
    QString m_osName;
    QString m_kernelVersion;
    QString m_architecture;
    QString m_prettyOsName;
    QString m_cpuModel;
    QString m_totalMemoryText;
    QStringList m_standardFolders;
};

} // namespace TagOS::System
