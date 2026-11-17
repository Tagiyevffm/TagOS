#include "tag-system/Metrics.h"

#include <QJsonValue>
#include <QString>

namespace TagOS::System {

quint64 MemoryInfo::usedBytes() const
{
    return totalBytes > availableBytes ? totalBytes - availableBytes : 0;
}

quint64 MemoryInfo::swapUsedBytes() const
{
    return swapTotalBytes > swapFreeBytes ? swapTotalBytes - swapFreeBytes : 0;
}

double MemoryInfo::usedPercent() const
{
    if (totalBytes == 0)
        return 0.0;
    return 100.0 * static_cast<double>(usedBytes()) / static_cast<double>(totalBytes);
}

quint64 DiskInfo::usedBytes() const
{
    return totalBytes > freeBytes ? totalBytes - freeBytes : 0;
}

double DiskInfo::usedPercent() const
{
    if (totalBytes == 0)
        return 0.0;
    return 100.0 * static_cast<double>(usedBytes()) / static_cast<double>(totalBytes);
}

namespace {

QJsonObject memoryToJson(const MemoryInfo &memory)
{
    return QJsonObject{
        {QStringLiteral("totalBytes"), static_cast<double>(memory.totalBytes)},
        {QStringLiteral("availableBytes"), static_cast<double>(memory.availableBytes)},
        {QStringLiteral("usedBytes"), static_cast<double>(memory.usedBytes())},
        {QStringLiteral("swapTotalBytes"), static_cast<double>(memory.swapTotalBytes)},
        {QStringLiteral("swapUsedBytes"), static_cast<double>(memory.swapUsedBytes())},
        {QStringLiteral("usedPercent"), memory.usedPercent()},
    };
}

MemoryInfo memoryFromJson(const QJsonObject &object)
{
    MemoryInfo memory;
    memory.totalBytes = static_cast<quint64>(object.value(QStringLiteral("totalBytes")).toDouble());
    memory.availableBytes = static_cast<quint64>(object.value(QStringLiteral("availableBytes")).toDouble());
    memory.swapTotalBytes = static_cast<quint64>(object.value(QStringLiteral("swapTotalBytes")).toDouble());
    memory.swapFreeBytes = memory.swapTotalBytes
        - static_cast<quint64>(object.value(QStringLiteral("swapUsedBytes")).toDouble());
    return memory;
}

} // namespace

QJsonObject toJson(const Snapshot &snapshot)
{
    QJsonObject load{
        {QStringLiteral("one"), snapshot.load.one},
        {QStringLiteral("five"), snapshot.load.five},
        {QStringLiteral("fifteen"), snapshot.load.fifteen},
        {QStringLiteral("available"), snapshot.load.available},
    };
    QJsonObject disk{
        {QStringLiteral("mountPoint"), snapshot.disk.mountPoint},
        {QStringLiteral("totalBytes"), static_cast<double>(snapshot.disk.totalBytes)},
        {QStringLiteral("freeBytes"), static_cast<double>(snapshot.disk.freeBytes)},
        {QStringLiteral("usedPercent"), snapshot.disk.usedPercent()},
    };

    return QJsonObject{
        {QStringLiteral("schema"), 1},
        {QStringLiteral("sampledAtMs"), static_cast<double>(snapshot.sampledAtMs)},
        {QStringLiteral("cpuPercent"), snapshot.cpuPercent},
        {QStringLiteral("cpuCount"), snapshot.cpuCount},
        {QStringLiteral("memory"), memoryToJson(snapshot.memory)},
        {QStringLiteral("swapUsedPercent"),
         100.0
             * static_cast<double>(snapshot.memory.swapUsedBytes())
             / static_cast<double>(qMax<quint64>(1, snapshot.memory.swapTotalBytes))},
        {QStringLiteral("load"), load},
        {QStringLiteral("disk"), disk},
        {QStringLiteral("uptimeSeconds"), static_cast<double>(snapshot.uptimeSeconds)},
        {QStringLiteral("hostName"), snapshot.hostName},
        {QStringLiteral("kernelVersion"), snapshot.kernelVersion},
        {QStringLiteral("arch"), snapshot.arch},
        {QStringLiteral("distroId"), snapshot.distroId},
        {QStringLiteral("distroName"), snapshot.distroName},
    };
}

Snapshot fromJson(const QJsonObject &object, bool *ok)
{
    Snapshot snapshot;
    bool valid = object.value(QStringLiteral("schema")).toInt() == 1;
    if (!valid) {
        if (ok)
            *ok = false;
        return snapshot;
    }

    snapshot.sampledAtMs = static_cast<qint64>(object.value(QStringLiteral("sampledAtMs")).toDouble());
    snapshot.cpuPercent = object.value(QStringLiteral("cpuPercent")).toDouble(-1.0);
    snapshot.cpuCount = object.value(QStringLiteral("cpuCount")).toInt();
    snapshot.memory = memoryFromJson(object.value(QStringLiteral("memory")).toObject());

    const QJsonObject load = object.value(QStringLiteral("load")).toObject();
    snapshot.load.one = load.value(QStringLiteral("one")).toDouble();
    snapshot.load.five = load.value(QStringLiteral("five")).toDouble();
    snapshot.load.fifteen = load.value(QStringLiteral("fifteen")).toDouble();
    snapshot.load.available = load.value(QStringLiteral("available")).toBool();

    const QJsonObject disk = object.value(QStringLiteral("disk")).toObject();
    snapshot.disk.mountPoint = disk.value(QStringLiteral("mountPoint")).toString();
    snapshot.disk.totalBytes = static_cast<quint64>(disk.value(QStringLiteral("totalBytes")).toDouble());
    snapshot.disk.freeBytes = static_cast<quint64>(disk.value(QStringLiteral("freeBytes")).toDouble());

    snapshot.uptimeSeconds = static_cast<qint64>(object.value(QStringLiteral("uptimeSeconds")).toDouble(-1));
    snapshot.hostName = object.value(QStringLiteral("hostName")).toString();
    snapshot.kernelVersion = object.value(QStringLiteral("kernelVersion")).toString();
    snapshot.arch = object.value(QStringLiteral("arch")).toString();
    snapshot.distroId = object.value(QStringLiteral("distroId")).toString();
    snapshot.distroName = object.value(QStringLiteral("distroName")).toString();

    if (ok)
        *ok = true;
    return snapshot;
}

} // namespace TagOS::System
