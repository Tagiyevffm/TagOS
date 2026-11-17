#pragma once

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

namespace TagOS::System {

/// Raw CPU time counters (the fields of the `/proc/stat` "cpu" row).
/// The Windows implementation maps `GetSystemTimes` onto the same fields so
/// that sampling math is identical on every platform.
struct CpuTimes
{
    quint64 user = 0;
    quint64 nice = 0;
    quint64 system = 0;
    quint64 idle = 0;
    quint64 iowait = 0;
    quint64 irq = 0;
    quint64 softirq = 0;
    quint64 steal = 0;
    quint64 guest = 0;
    quint64 guestNice = 0;

    quint64 total() const
    {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    quint64 busy() const { return total() - idle - iowait; }
    bool operator==(const CpuTimes &o) const { return total() == o.total() && busy() == o.busy(); }
};

struct MemoryInfo
{
    quint64 totalBytes = 0;
    quint64 availableBytes = 0;
    quint64 swapTotalBytes = 0;
    quint64 swapFreeBytes = 0;

    /// `MemTotal - MemAvailable`, i.e. what the kernel considers in use.
    quint64 usedBytes() const;
    quint64 swapUsedBytes() const;
    double usedPercent() const;
};

struct LoadAverage
{
    double one = 0.0;
    double five = 0.0;
    double fifteen = 0.0;
    /// Windows has no load average; the field stays false there.
    bool available = false;
};

struct DiskInfo
{
    QString mountPoint = QStringLiteral("/");
    quint64 totalBytes = 0;
    quint64 freeBytes = 0;

    quint64 usedBytes() const;
    double usedPercent() const;
};

/// One immutable reading of the local system.
struct Snapshot
{
    qint64 sampledAtMs = 0;
    /// -1 until two CPU samples are available.
    double cpuPercent = -1.0;
    MemoryInfo memory;
    LoadAverage load;
    DiskInfo disk;
    qint64 uptimeSeconds = -1;
    int cpuCount = 0;
    QString hostName;
    QString kernelVersion;
    QString arch;
    QString distroId;
    QString distroName;
};

QJsonObject toJson(const Snapshot &snapshot);
Snapshot fromJson(const QJsonObject &object, bool *ok = nullptr);

} // namespace TagOS::System
