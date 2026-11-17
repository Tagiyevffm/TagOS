// Windows developer-host probe.
//
// TagOS itself targets Arch Linux; this backend exists so the desktop shell can
// be developed and smoke-tested on a Windows workstation without a VM. It maps
// the Win32 counters onto the same CpuTimes fields as procfs, so the sampling
// math and the UI are identical.
#include "tag-system/SystemProbe.h"

#include "TagOSConfig.h"
#include "tag-system/CpuUsageSampler.h"

#include <QDateTime>
#include <QSysInfo>
#include <QThread>

#include <windows.h>

namespace TagOS::System {
namespace {

#define TAGOS_FILE_TO_U64(ft) (static_cast<quint64>((ft).dwHighDateTime) << 32 | static_cast<quint64>((ft).dwLowDateTime))

CpuUsageSampler &sampler()
{
    static CpuUsageSampler instance;
    return instance;
}

CpuTimes readCpuTimes()
{
    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (!::GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return {};

    // Kernel time already contains idle time on Windows.
    const quint64 idle = TAGOS_FILE_TO_U64(idleTime);
    const quint64 kernel = TAGOS_FILE_TO_U64(kernelTime);
    const quint64 user = TAGOS_FILE_TO_U64(userTime);

    CpuTimes times;
    times.user = user;
    times.system = kernel > idle ? kernel - idle : 0;
    times.idle = idle;
    return times;
}

MemoryInfo readMemory()
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!::GlobalMemoryStatusEx(&status))
        return {};
    MemoryInfo memory;
    memory.totalBytes = status.ullTotalPhys;
    memory.availableBytes = status.ullAvailPhys;
    // Windows reports a commit limit, not a swap device. Reporting it as swap
    // would produce a meaningless percentage, so dev hosts report "no swap".
    memory.swapTotalBytes = 0;
    memory.swapFreeBytes = 0;
    return memory;
}

DiskInfo readDisk()
{
    DiskInfo disk;
    disk.mountPoint = QStringLiteral("C:/");
    ULARGE_INTEGER freeBytes{};
    ULARGE_INTEGER totalBytes{};
    if (!::GetDiskFreeSpaceExW(L"C:\\", nullptr, &totalBytes, &freeBytes))
        return disk;
    disk.totalBytes = totalBytes.QuadPart;
    disk.freeBytes = freeBytes.QuadPart;
    return disk;
}

} // namespace

Snapshot sampleSystem()
{
    Snapshot snapshot;
    snapshot.sampledAtMs = QDateTime::currentMSecsSinceEpoch();
    snapshot.cpuPercent = sampler().add(readCpuTimes());
    snapshot.memory = readMemory();
    snapshot.disk = readDisk();
    snapshot.load.available = false;
    snapshot.uptimeSeconds = static_cast<qint64>(::GetTickCount64() / 1000ULL);
    snapshot.cpuCount = QThread::idealThreadCount();
    snapshot.hostName = QSysInfo::machineHostName();
    snapshot.kernelVersion = QSysInfo::kernelVersion();
    snapshot.arch = QSysInfo::currentCpuArchitecture();
    snapshot.distroId = QStringLiteral("windows-dev");
    snapshot.distroName = QStringLiteral("Windows (dev host)");
    return snapshot;
}

QString tagosVersion()
{
    return QStringLiteral(TAGOS_DISPLAY_VERSION);
}

QString platformDisplayName()
{
    return QStringLiteral("Windows (dev host)");
}

} // namespace TagOS::System
