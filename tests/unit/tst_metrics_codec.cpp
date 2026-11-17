#include "tag-system/Metrics.h"

#include <QJsonDocument>
#include <QTest>

using namespace TagOS::System;

class MetricsCodecTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsThroughJson();
    void rejectsUnknownSchema();
    void computesDerivedValues();
};

namespace {
Snapshot makeSnapshot()
{
    Snapshot snapshot;
    snapshot.sampledAtMs = 1717000000123;
    snapshot.cpuPercent = 37.5;
    snapshot.cpuCount = 4;
    snapshot.memory.totalBytes = 4ull * 1024 * 1024 * 1024;
    snapshot.memory.availableBytes = 1ull * 1024 * 1024 * 1024;
    snapshot.memory.swapTotalBytes = 2ull * 1024 * 1024 * 1024;
    snapshot.memory.swapFreeBytes = 2ull * 1024 * 1024 * 1024;
    snapshot.load.one = 0.5;
    snapshot.load.five = 0.4;
    snapshot.load.fifteen = 0.3;
    snapshot.load.available = true;
    snapshot.disk.mountPoint = QStringLiteral("/");
    snapshot.disk.totalBytes = 100ull * 1024 * 1024 * 1024;
    snapshot.disk.freeBytes = 25ull * 1024 * 1024 * 1024;
    snapshot.uptimeSeconds = 3725;
    snapshot.hostName = QStringLiteral("tagos-vm");
    snapshot.kernelVersion = QStringLiteral("6.16.1-arch1-1");
    snapshot.arch = QStringLiteral("x86_64");
    snapshot.distroId = QStringLiteral("arch");
    snapshot.distroName = QStringLiteral("Arch Linux");
    return snapshot;
}
} // namespace

void MetricsCodecTest::roundTripsThroughJson()
{
    const Snapshot original = makeSnapshot();

    // Serialise and deserialise through real JSON text, not just QJsonObject.
    const QByteArray encoded = QJsonDocument(toJson(original)).toJson(QJsonDocument::Compact);
    const QJsonObject object = QJsonDocument::fromJson(encoded).object();

    bool ok = false;
    const Snapshot decoded = fromJson(object, &ok);

    QVERIFY(ok);
    QCOMPARE(decoded.sampledAtMs, original.sampledAtMs);
    QCOMPARE(decoded.cpuPercent, original.cpuPercent);
    QCOMPARE(decoded.cpuCount, original.cpuCount);
    QCOMPARE(decoded.memory.totalBytes, original.memory.totalBytes);
    QCOMPARE(decoded.memory.availableBytes, original.memory.availableBytes);
    QCOMPARE(decoded.memory.swapTotalBytes, original.memory.swapTotalBytes);
    QCOMPARE(decoded.load.one, original.load.one);
    QVERIFY(decoded.load.available);
    QCOMPARE(decoded.disk.totalBytes, original.disk.totalBytes);
    QCOMPARE(decoded.disk.freeBytes, original.disk.freeBytes);
    QCOMPARE(decoded.uptimeSeconds, original.uptimeSeconds);
    QCOMPARE(decoded.hostName, original.hostName);
    QCOMPARE(decoded.kernelVersion, original.kernelVersion);
    QCOMPARE(decoded.distroName, original.distroName);
    QCOMPARE(decoded.memory.usedPercent(), original.memory.usedPercent());
    QCOMPARE(decoded.disk.usedPercent(), original.disk.usedPercent());
}

void MetricsCodecTest::rejectsUnknownSchema()
{
    QJsonObject object = toJson(makeSnapshot());
    object.insert(QStringLiteral("schema"), 99);

    bool ok = true;
    fromJson(object, &ok);
    QVERIFY(!ok);
}

void MetricsCodecTest::computesDerivedValues()
{
    const Snapshot snapshot = makeSnapshot();
    QCOMPARE(snapshot.memory.usedBytes(), 3ull * 1024 * 1024 * 1024);
    QCOMPARE(snapshot.memory.usedPercent(), 75.0);
    QCOMPARE(snapshot.disk.usedPercent(), 75.0);
    QCOMPARE(snapshot.memory.swapUsedBytes(), quint64(0));
}

QTEST_GUILESS_MAIN(MetricsCodecTest)
#include "tst_metrics_codec.moc"
