// Tests the TagFlow wire format — the QVariantMap that tag-flowd sends over
// D-Bus and that FlowClient decodes. A schema mismatch must be detected instead
// of silently producing blank gauges.
#include "tagflow/TagFlowTypes.h"

#include <QTest>

using namespace TagOS;

class FlowWireTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsSnapshot();
    void rejectsWrongSchema();
    void transportNames();
};

namespace {
System::Snapshot makeSnapshot()
{
    System::Snapshot snapshot;
    snapshot.sampledAtMs = 1717000000999;
    snapshot.cpuPercent = 12.25;
    snapshot.cpuCount = 8;
    snapshot.memory.totalBytes = 8ull * 1024 * 1024 * 1024;
    snapshot.memory.availableBytes = 6ull * 1024 * 1024 * 1024;
    snapshot.memory.swapTotalBytes = 4ull * 1024 * 1024 * 1024;
    snapshot.memory.swapFreeBytes = 3ull * 1024 * 1024 * 1024;
    snapshot.load.one = 1.5;
    snapshot.load.five = 1.25;
    snapshot.load.fifteen = 1.0;
    snapshot.load.available = true;
    snapshot.disk.mountPoint = QStringLiteral("/");
    snapshot.disk.totalBytes = 32ull * 1024 * 1024 * 1024;
    snapshot.disk.freeBytes = 8ull * 1024 * 1024 * 1024;
    snapshot.uptimeSeconds = 1234;
    snapshot.hostName = QStringLiteral("tagos-vm");
    snapshot.kernelVersion = QStringLiteral("6.16.1-arch1-1");
    snapshot.arch = QStringLiteral("x86_64");
    snapshot.distroId = QStringLiteral("tagos");
    snapshot.distroName = QStringLiteral("TagOS Linux");
    return snapshot;
}
} // namespace

void FlowWireTest::roundTripsSnapshot()
{
    const System::Snapshot original = makeSnapshot();
    const QVariantMap map = Flow::toVariantMap(original);

    QCOMPARE(map.value(QStringLiteral("schema")).toInt(), Flow::kSchemaVersion);

    bool ok = false;
    const System::Snapshot decoded = Flow::snapshotFromVariantMap(map, &ok);

    QVERIFY(ok);
    QCOMPARE(decoded.sampledAtMs, original.sampledAtMs);
    QCOMPARE(decoded.cpuPercent, original.cpuPercent);
    QCOMPARE(decoded.cpuCount, original.cpuCount);
    QCOMPARE(decoded.memory.totalBytes, original.memory.totalBytes);
    QCOMPARE(decoded.memory.availableBytes, original.memory.availableBytes);
    QCOMPARE(decoded.memory.swapUsedBytes(), original.memory.swapUsedBytes());
    QCOMPARE(decoded.load.fifteen, original.load.fifteen);
    QCOMPARE(decoded.disk.freeBytes, original.disk.freeBytes);
    QCOMPARE(decoded.uptimeSeconds, original.uptimeSeconds);
    QCOMPARE(decoded.hostName, original.hostName);
    QCOMPARE(decoded.distroId, original.distroId);
}

void FlowWireTest::rejectsWrongSchema()
{
    QVariantMap map = Flow::toVariantMap(makeSnapshot());
    map.insert(QStringLiteral("schema"), 42);

    bool ok = true;
    Flow::snapshotFromVariantMap(map, &ok);
    QVERIFY(!ok);
}

void FlowWireTest::transportNames()
{
    QCOMPARE(Flow::transportId(Flow::Transport::Dbus), QStringLiteral("dbus"));
    QCOMPARE(Flow::transportId(Flow::Transport::Local), QStringLiteral("local"));
    QCOMPARE(Flow::transportLabel(Flow::Transport::Dbus), QStringLiteral("TagFlow (D-Bus)"));
    QCOMPARE(Flow::transportLabel(Flow::Transport::Local), QStringLiteral("Direct sampling"));
}

QTEST_GUILESS_MAIN(FlowWireTest)
#include "tst_flow_wire.moc"
