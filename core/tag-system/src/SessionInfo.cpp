#include "tag-system/SessionInfo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QSysInfo>
#include <QRegularExpression>

#include "tag-system/OsRelease.h"

namespace TagOS::System {
namespace {

QString readFirstLine(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readLine()).trimmed();
}

QString cpuModelFromCpuinfo()
{
    QFile file(QStringLiteral("/proc/cpuinfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if (line.startsWith(QLatin1String("model name")) || line.startsWith(QLatin1String("Model"))) {
            const int colon = line.indexOf(QLatin1Char(':'));
            if (colon > 0)
                return line.mid(colon + 1).trimmed();
        }
    }
    return QString();
}

QString memoryTotalFromProc()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if (line.startsWith(QLatin1String("MemTotal:"))) {
            static const QRegularExpression re(QStringLiteral(R"((\d+)\s*kB)"));
            const auto match = re.match(line);
            if (match.hasMatch()) {
                const double gib = match.captured(1).toDouble() / (1024.0 * 1024.0);
                return QString::number(gib, 'f', 1) + QStringLiteral(" GiB");
            }
        }
    }
    return QString();
}

} // namespace

SessionInfo::SessionInfo(QObject *parent)
    : QObject(parent)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    m_userName = env.value(QStringLiteral("USER"));
    if (m_userName.isEmpty())
        m_userName = env.value(QStringLiteral("USERNAME"));
    m_homePath = QDir::homePath();
    m_shellPath = env.value(QStringLiteral("SHELL"));
    if (m_shellPath.isEmpty())
        m_shellPath = QStringLiteral("/bin/sh");
    m_hostName = QSysInfo::machineHostName();
    m_desktopName = env.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    if (m_desktopName.isEmpty())
        m_desktopName = QStringLiteral("TagOS");
    m_sessionType = env.value(QStringLiteral("XDG_SESSION_TYPE"));
    if (m_sessionType.isEmpty())
        m_sessionType = QStringLiteral("wayland");
    m_localeName = env.value(QStringLiteral("LANG"));
    if (m_localeName.isEmpty())
        m_localeName = QStringLiteral("tr_TR.UTF-8");

    m_architecture = QSysInfo::currentCpuArchitecture();
    m_kernelVersion = QSysInfo::kernelVersion();
    m_osName = QSysInfo::productType();

    const OsRelease release = readOsRelease(QStringLiteral("/etc/os-release"));
    if (release.isValid())
        m_prettyOsName = release.prettyName;
    else
        m_prettyOsName = QSysInfo::prettyProductName();

    m_cpuModel = cpuModelFromCpuinfo();
    if (m_cpuModel.isEmpty())
        m_cpuModel = QSysInfo::currentCpuArchitecture();

    m_totalMemoryText = memoryTotalFromProc();

    const QStandardPaths::StandardLocation folders[] = {
        QStandardPaths::HomeLocation,      QStandardPaths::DesktopLocation,
        QStandardPaths::DocumentsLocation, QStandardPaths::DownloadLocation,
        QStandardPaths::PicturesLocation,  QStandardPaths::MusicLocation,
        QStandardPaths::MoviesLocation,
    };
    for (const auto folder : folders) {
        const QString path = QStandardPaths::writableLocation(folder);
        if (!path.isEmpty() && !m_standardFolders.contains(path))
            m_standardFolders.append(path);
    }
}

QString SessionInfo::tagosVersion() const
{
#ifdef TAGOS_VERSION
    return QStringLiteral(TAGOS_VERSION);
#else
    return QStringLiteral("0.0.0");
#endif
}

QString SessionInfo::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

QString SessionInfo::buildType() const
{
#ifdef QT_NO_DEBUG
    return QStringLiteral("release");
#else
    return QStringLiteral("debug");
#endif
}

} // namespace TagOS::System
