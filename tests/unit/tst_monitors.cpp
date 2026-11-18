#include "tag-system/Monitors.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QTest>
#include <QThread>

using namespace TagOS::System;

namespace {

void writeText(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
}

QByteArray processStat(int pid, const QString &name, quint64 utime, quint64 stime, quint64 rssPages)
{
    // Fields follow proc(5): state, ppid…, utime (12th), stime (13th), rss (24th).
    return QStringLiteral("%1 (%2) S 1 %1 %1 0 -1 4194560 100 0 0 0 %3 %4 0 0 20 0 1 0 1000 0 %5")
        .arg(pid)
        .arg(name)
        .arg(utime)
        .arg(stime)
        .arg(rssPages)
        .toUtf8();
}

} // namespace

class MonitorsTest : public QObject
{
    Q_OBJECT

private slots:
    void processListReadsSummary();
    void processListComputesCpuDelta();
    void networkSamplerTotals();
    void networkSamplerPicksBusiestInterface();
    void thermalReaderScalesMillidegrees();
    void unavailableRootsReportUnavailable();
};

void MonitorsTest::processListReadsSummary()
{
    QTemporaryDir proc;
    QVERIFY(proc.isValid());
    writeText(proc.filePath("stat"), "cpu  100 0 100 800 0 0 0 0 0 0\n");
    writeText(proc.filePath("meminfo"), "MemTotal:        8000000 kB\nSwapTotal: 0 kB\n");
    writeText(proc.filePath("1234/stat"), processStat(1234, "tag-shell", 50, 30, 1024));
    writeText(proc.filePath("1234/cmdline"), "/usr/bin/tag-shell\0--ready-file\0");
    writeText(proc.filePath("99/stat"), processStat(99, "tag-flowd", 10, 5, 256));

    ProcessList list(nullptr, proc.path());
    QVERIFY(list.available());
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.processCount(), 2);
    QVERIFY(list.summary().contains(QStringLiteral("2")));

    const QVariantMap first = list.at(0);
    QVERIFY(!first.value(QStringLiteral("name")).toString().isEmpty());
    QVERIFY(first.contains(QStringLiteral("memoryText")));
}

void MonitorsTest::processListComputesCpuDelta()
{
    QTemporaryDir proc;
    QVERIFY(proc.isValid());
    writeText(proc.filePath("stat"), "cpu  100 0 100 800 0 0 0 0 0 0\n");
    writeText(proc.filePath("meminfo"), "MemTotal:        8000000 kB\n");
    writeText(proc.filePath("4242/stat"), processStat(4242, "busy", 100, 0, 512));

    ProcessList list(nullptr, proc.path());
    QCOMPARE(list.at(0).value(QStringLiteral("cpu")).toDouble(), 0.0);

    // Total ticks +200, this process +100 -> 50% of the machine.
    writeText(proc.filePath("stat"), "cpu  200 0 100 900 0 0 0 0 0 0\n");
    writeText(proc.filePath("4242/stat"), processStat(4242, "busy", 200, 0, 512));
    list.refresh();

    const double cpu = list.at(0).value(QStringLiteral("cpu")).toDouble();
    QVERIFY(cpu > 0.0);
    QVERIFY(cpu <= 100.0 * QThread::idealThreadCount());
}

void MonitorsTest::networkSamplerTotals()
{
    QTemporaryDir proc;
    QVERIFY(proc.isValid());
    writeText(proc.filePath("net/dev"),
              "Inter-|   Receive                                                |  Transmit\n"
              " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets\n"
              "    lo: 9999 0 0 0 0 0 0 0 9999 0 0 0 0 0 0 0\n"
              "  eth0: 4096 10 0 0 0 0 0 0 8192 20 0 0 0 0 0 0\n");

    NetworkSampler sampler(nullptr, proc.path());
    QVERIFY(sampler.available());
    QCOMPARE(sampler.interfaceName(), QStringLiteral("eth0"));
    QCOMPARE(sampler.rxTotalText(), QStringLiteral("4 KiB"));
    QCOMPARE(sampler.txTotalText(), QStringLiteral("8 KiB"));
    QVERIFY(!sampler.rxRateText().isEmpty());
}

void MonitorsTest::networkSamplerPicksBusiestInterface()
{
    QTemporaryDir proc;
    QVERIFY(proc.isValid());
    writeText(proc.filePath("net/dev"),
              "  eth0: 100 0 0 0 0 0 0 0 100 0 0 0 0 0 0 0\n"
              "  wlan0: 900000 0 0 0 0 0 0 0 500000 0 0 0 0 0 0 0\n");

    NetworkSampler sampler(nullptr, proc.path());
    QCOMPARE(sampler.interfaceName(), QStringLiteral("wlan0"));
}

void MonitorsTest::thermalReaderScalesMillidegrees()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    writeText(root.filePath("thermal_zone0/temp"), "45000\n");
    writeText(root.filePath("thermal_zone0/type"), "x86_pkg_temp\n");
    writeText(root.filePath("thermal_zone1/temp"), "38\n");
    writeText(root.filePath("thermal_zone1/type"), "acpitz\n");

    ThermalReader thermal(nullptr, root.path());
    QVERIFY(thermal.available());
    QCOMPARE(thermal.celsius(), 45.0);
    QCOMPARE(thermal.label(), QStringLiteral("x86_pkg_temp"));
    QVERIFY(thermal.text().contains(QStringLiteral("45")));
}

void MonitorsTest::unavailableRootsReportUnavailable()
{
    ProcessList list(nullptr, QStringLiteral("/nonexistent-proc"));
    QVERIFY(!list.available());
    QCOMPARE(list.count(), 0);

    NetworkSampler sampler(nullptr, QStringLiteral("/nonexistent-proc"));
    QVERIFY(!sampler.available());

    ThermalReader thermal(nullptr, QStringLiteral("/nonexistent-thermal"));
    QVERIFY(!thermal.available());
}

QTEST_GUILESS_MAIN(MonitorsTest)
#include "tst_monitors.moc"
