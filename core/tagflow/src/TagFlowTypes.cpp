#include "tagflow/TagFlowTypes.h"

namespace TagOS::Flow {

QString transportId(Transport transport)
{
    switch (transport) {
    case Transport::Dbus:
        return QStringLiteral("dbus");
    case Transport::Local:
        return QStringLiteral("local");
    }
    return QStringLiteral("unknown");
}

QString transportLabel(Transport transport)
{
    switch (transport) {
    case Transport::Dbus:
        return QStringLiteral("TagFlow (D-Bus)");
    case Transport::Local:
        return QStringLiteral("Direct sampling");
    }
    return QStringLiteral("Unavailable");
}

QVariantMap toVariantMap(const TagOS::System::Snapshot &snapshot)
{
    return QVariantMap{
        {QStringLiteral("schema"), kSchemaVersion},
        {QStringLiteral("sampledAtMs"), static_cast<qlonglong>(snapshot.sampledAtMs)},
        {QStringLiteral("cpuPercent"), snapshot.cpuPercent},
        {QStringLiteral("cpuCount"), snapshot.cpuCount},
        {QStringLiteral("memoryTotalBytes"), static_cast<qulonglong>(snapshot.memory.totalBytes)},
        {QStringLiteral("memoryAvailableBytes"), static_cast<qulonglong>(snapshot.memory.availableBytes)},
        {QStringLiteral("swapTotalBytes"), static_cast<qulonglong>(snapshot.memory.swapTotalBytes)},
        {QStringLiteral("swapFreeBytes"), static_cast<qulonglong>(snapshot.memory.swapFreeBytes)},
        {QStringLiteral("loadOne"), snapshot.load.one},
        {QStringLiteral("loadFive"), snapshot.load.five},
        {QStringLiteral("loadFifteen"), snapshot.load.fifteen},
        {QStringLiteral("loadAvailable"), snapshot.load.available},
        {QStringLiteral("diskMountPoint"), snapshot.disk.mountPoint},
        {QStringLiteral("diskTotalBytes"), static_cast<qulonglong>(snapshot.disk.totalBytes)},
        {QStringLiteral("diskFreeBytes"), static_cast<qulonglong>(snapshot.disk.freeBytes)},
        {QStringLiteral("uptimeSeconds"), static_cast<qlonglong>(snapshot.uptimeSeconds)},
        {QStringLiteral("hostName"), snapshot.hostName},
        {QStringLiteral("kernelVersion"), snapshot.kernelVersion},
        {QStringLiteral("arch"), snapshot.arch},
        {QStringLiteral("distroId"), snapshot.distroId},
        {QStringLiteral("distroName"), snapshot.distroName},
    };
}

TagOS::System::Snapshot snapshotFromVariantMap(const QVariantMap &map, bool *ok)
{
    TagOS::System::Snapshot snapshot;
    const int schema = map.value(QStringLiteral("schema"), 0).toInt();
    if (schema != kSchemaVersion) {
        if (ok)
            *ok = false;
        return snapshot;
    }

    snapshot.sampledAtMs = map.value(QStringLiteral("sampledAtMs")).toLongLong();
    snapshot.cpuPercent = map.value(QStringLiteral("cpuPercent"), -1.0).toDouble();
    snapshot.cpuCount = map.value(QStringLiteral("cpuCount")).toInt();
    snapshot.memory.totalBytes = map.value(QStringLiteral("memoryTotalBytes")).toULongLong();
    snapshot.memory.availableBytes = map.value(QStringLiteral("memoryAvailableBytes")).toULongLong();
    snapshot.memory.swapTotalBytes = map.value(QStringLiteral("swapTotalBytes")).toULongLong();
    snapshot.memory.swapFreeBytes = map.value(QStringLiteral("swapFreeBytes")).toULongLong();
    snapshot.load.one = map.value(QStringLiteral("loadOne")).toDouble();
    snapshot.load.five = map.value(QStringLiteral("loadFive")).toDouble();
    snapshot.load.fifteen = map.value(QStringLiteral("loadFifteen")).toDouble();
    snapshot.load.available = map.value(QStringLiteral("loadAvailable")).toBool();
    snapshot.disk.mountPoint = map.value(QStringLiteral("diskMountPoint")).toString();
    snapshot.disk.totalBytes = map.value(QStringLiteral("diskTotalBytes")).toULongLong();
    snapshot.disk.freeBytes = map.value(QStringLiteral("diskFreeBytes")).toULongLong();
    snapshot.uptimeSeconds = map.value(QStringLiteral("uptimeSeconds"), -1).toLongLong();
    snapshot.hostName = map.value(QStringLiteral("hostName")).toString();
    snapshot.kernelVersion = map.value(QStringLiteral("kernelVersion")).toString();
    snapshot.arch = map.value(QStringLiteral("arch")).toString();
    snapshot.distroId = map.value(QStringLiteral("distroId")).toString();
    snapshot.distroName = map.value(QStringLiteral("distroName")).toString();

    if (ok)
        *ok = true;
    return snapshot;
}

} // namespace TagOS::Flow
