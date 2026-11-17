#include "tag-system/Format.h"

#include <QTest>

using namespace TagOS::System;

class FormatTest : public QObject
{
    Q_OBJECT

private slots:
    void bytes();
    void uptime();
    void percent();
    void load();
};

void FormatTest::bytes()
{
    QCOMPARE(formatBytes(0), QStringLiteral("0 B"));
    QCOMPARE(formatBytes(512), QStringLiteral("512 B"));
    QCOMPARE(formatBytes(1024), QStringLiteral("1 KiB"));
    QCOMPARE(formatBytes(1536), QStringLiteral("1.5 KiB"));
    QCOMPARE(formatBytes(1024ull * 1024 * 1024 * 3), QStringLiteral("3 GiB"));
    QCOMPARE(formatBytes(1024ull * 1024 * 1024 * 1.25, 2), QStringLiteral("1.25 GiB"));
    QCOMPARE(formatBytes(quint64(2) * 1024 * 1024, 0), QStringLiteral("2 MiB"));
}

void FormatTest::uptime()
{
    QCOMPARE(formatUptime(-1), QStringLiteral("--"));
    QCOMPARE(formatUptime(45), QStringLiteral("0:45"));
    QCOMPARE(formatUptime(3725), QStringLiteral("1:02:05"));
    QCOMPARE(formatUptime(90000), QStringLiteral("1d 1:00"));
}

void FormatTest::percent()
{
    QCOMPARE(formatPercent(-1.0), QStringLiteral("--"));
    QCOMPARE(formatPercent(0.0), QStringLiteral("0%"));
    QCOMPARE(formatPercent(24.4), QStringLiteral("24%"));
    QCOMPARE(formatPercent(24.44, 1), QStringLiteral("24.4%"));
    QCOMPARE(formatPercent(100.0, 2), QStringLiteral("100%"));
}

void FormatTest::load()
{
    LoadAverage load;
    QCOMPARE(formatLoad(load), QStringLiteral("--"));

    load.available = true;
    load.one = 0.5;
    load.five = 0.25;
    load.fifteen = 0.125;
    QCOMPARE(formatLoad(load), QStringLiteral("0.50 0.25 0.13"));
}

QTEST_GUILESS_MAIN(FormatTest)
#include "tst_format.moc"
