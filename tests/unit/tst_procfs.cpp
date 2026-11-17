#include "tag-system/ProcFs.h"

#include <QDir>
#include <QTest>

using namespace TagOS::System;

class ProcFsTest : public QObject
{
    Q_OBJECT

private slots:
    void cpuTimesParsesFirstRow();
    void cpuTimesReportsFailureForMissingFile();
    void memoryInfoConvertsKilobytes();
    void memoryInfoFallsBackToFreeBuffersCached();
    void loadAverageParsesThreeValues();
    void uptimeSecondsTruncates();
};

namespace {
QString fixture(const QString &relative)
{
    return QStringLiteral(TAGOS_FIXTURE_DIR) + QLatin1Char('/') + relative;
}
} // namespace

void ProcFsTest::cpuTimesParsesFirstRow()
{
    const ProcFs proc(fixture(QStringLiteral("proc")));
    bool ok = false;
    const CpuTimes times = proc.cpuTimes(&ok);

    QVERIFY(ok);
    QCOMPARE(times.user, quint64(123456));
    QCOMPARE(times.nice, quint64(4567));
    QCOMPARE(times.system, quint64(98765));
    QCOMPARE(times.idle, quint64(4567890));
    QCOMPARE(times.iowait, quint64(12345));
    QCOMPARE(times.irq, quint64(0));
    QCOMPARE(times.softirq, quint64(4321));

    // total = user+nice+system+idle+iowait+irq+softirq+steal
    QCOMPARE(times.total(), quint64(123456 + 4567 + 98765 + 4567890 + 12345 + 0 + 4321 + 0));
    QCOMPARE(times.busy(), times.total() - times.idle - times.iowait);
}

void ProcFsTest::cpuTimesReportsFailureForMissingFile()
{
    const ProcFs proc(fixture(QStringLiteral("does-not-exist")));
    bool ok = true;
    const CpuTimes times = proc.cpuTimes(&ok);
    QVERIFY(!ok);
    QCOMPARE(times.total(), quint64(0));
}

void ProcFsTest::memoryInfoConvertsKilobytes()
{
    const ProcFs proc(fixture(QStringLiteral("proc")));
    bool ok = false;
    const MemoryInfo memory = proc.memoryInfo(&ok);

    QVERIFY(ok);
    QCOMPARE(memory.totalBytes, quint64(4000000) * 1024);
    QCOMPARE(memory.availableBytes, quint64(1600000) * 1024);
    QCOMPARE(memory.swapTotalBytes, quint64(2000000) * 1024);
    QCOMPARE(memory.swapFreeBytes, quint64(1500000) * 1024);
    QCOMPARE(memory.usedBytes(), (quint64(4000000) - quint64(1600000)) * 1024);
    QVERIFY(qAbs(memory.usedPercent() - 60.0) < 0.001);
    QCOMPARE(memory.swapUsedBytes(), quint64(500000) * 1024);
}

void ProcFsTest::memoryInfoFallsBackToFreeBuffersCached()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath(QStringLiteral("meminfo")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("MemTotal: 1000 kB\n"
               "MemFree: 100 kB\n"
               "Buffers: 50 kB\n"
               "Cached: 150 kB\n");
    file.close();

    bool ok = false;
    const MemoryInfo memory = ProcFs(dir.path()).memoryInfo(&ok);
    QVERIFY(ok);
    // 100 + 50 + 150 KiB available without MemAvailable.
    QCOMPARE(memory.availableBytes, quint64(300) * 1024);
}

void ProcFsTest::loadAverageParsesThreeValues()
{
    bool ok = false;
    const LoadAverage load = ProcFs(fixture(QStringLiteral("proc"))).loadAverage(&ok);
    QVERIFY(ok);
    QVERIFY(load.available);
    QCOMPARE(load.one, 0.52);
    QCOMPARE(load.five, 0.41);
    QCOMPARE(load.fifteen, 0.38);
}

void ProcFsTest::uptimeSecondsTruncates()
{
    bool ok = false;
    QCOMPARE(ProcFs(fixture(QStringLiteral("proc"))).uptimeSeconds(&ok), qint64(3725));
    QVERIFY(ok);
}

QTEST_GUILESS_MAIN(ProcFsTest)
#include "tst_procfs.moc"
