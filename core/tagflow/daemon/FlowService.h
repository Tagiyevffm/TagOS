#pragma once

#include <QDBusContext>
#include <QObject>
#include <QVariantMap>

#include "tag-system/Metrics.h"

namespace TagOS::Flow {

/// The object tag-flowd exports as `org.tagos.Flow1` on the session bus.
///
/// Members mirror `core/tagflow/dbus/org.tagos.Flow1.xml` — the XML is the
/// contract, this class is the implementation.
class FlowService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.tagos.Flow1")

    Q_PROPERTY(double CpuPercent READ cpuPercent)
    Q_PROPERTY(double MemoryUsedPercent READ memoryUsedPercent)
    Q_PROPERTY(double DiskUsedPercent READ diskUsedPercent)
    Q_PROPERTY(qlonglong UptimeSeconds READ uptimeSeconds)

public:
    explicit FlowService(QObject *parent = nullptr);

    /// Re-samples and broadcasts `SnapshotChanged`.
    void poll();

    double cpuPercent() const { return m_snapshot.cpuPercent; }
    double memoryUsedPercent() const { return m_snapshot.memory.usedPercent(); }
    double diskUsedPercent() const { return m_snapshot.disk.usedPercent(); }
    qlonglong uptimeSeconds() const { return m_snapshot.uptimeSeconds; }

public slots:
    QVariantMap Snapshot() const;
    bool Ping() const { return true; }
    QString Version() const;

signals:
    void SnapshotChanged(const QVariantMap &snapshot);

private:
    TagOS::System::Snapshot m_snapshot;
};

} // namespace TagOS::Flow
