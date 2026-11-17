#include "tag-system/SystemProbe.h"

#include "TagOSConfig.h"
#include "tag-system/CpuUsageSampler.h"
#include "tag-system/OsRelease.h"
#include "tag-system/ProcFs.h"

#include <QDateTime>
#include <QFile>
#include <QSysInfo>
#include <QThread>

#include <sys/statvfs.h>

namespace TagOS::System {
namespace {

// A single process-wide sampler: the shell, the metrics CLI and tag-flowd each
// live in their own process, so one static per process is exactly right.
CpuUsageSampler &sampler()
{
    static CpuUsageSampler instance;
    return instance;
}

DiskInfo sampleDisk(const QString &mountPoint)
{
    DiskInfo disk;
    disk.mountPoint = mountPoint;
    struct statvfs stats = {};
    if (::statvfs(QFile::encodeName(mountPoint).constData(), &stats) != 0)
        return disk;
    disk.totalBytes = static_cast<quint64>(stats.f_blocks) * stats.f_frsize;
    disk.freeBytes = static_cast<quint64>(stats.f_bavail) * stats.f_frsize;
    return disk;
}

} // namespace

Snapshot sampleSystem()
{
    Snapshot snapshot;
    snapshot.sampledAtMs = QDateTime::currentMSecsSinceEpoch();

    const ProcFs proc(QStringLiteral("/proc"));
    bool ok = false;

    snapshot.cpuPercent = sampler().add(proc.cpuTimes(&ok));
    if (!ok)
        snapshot.cpuPercent = -1.0;
    snapshot.memory = proc.memoryInfo();
    snapshot.load = proc.loadAverage();
    snapshot.uptimeSeconds = proc.uptimeSeconds();
    snapshot.disk = sampleDisk(QStringLiteral("/"));
    snapshot.cpuCount = QThread::idealThreadCount();

    snapshot.hostName = QSysInfo::machineHostName();
    snapshot.kernelVersion = QSysInfo::kernelVersion();
    snapshot.arch = QSysInfo::currentCpuArchitecture();

    // Prefer the TagOS marker when the image ships one, then os-release.
    OsRelease release = readOsRelease(QStringLiteral("/etc/tagos-release"));
    if (!release.isValid())
        release = readOsRelease(QStringLiteral("/etc/os-release"));
    snapshot.distroId = release.id;
    snapshot.distroName = release.displayName();
    return snapshot;
}

QString tagosVersion()
{
    return QStringLiteral(TAGOS_DISPLAY_VERSION);
}

QString platformDisplayName()
{
    const OsRelease release = readOsRelease(QStringLiteral("/etc/os-release"));
    return release.isValid() ? release.displayName() : QStringLiteral("Linux");
}

} // namespace TagOS::System
