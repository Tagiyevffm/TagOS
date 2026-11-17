#include "tag-system/CpuUsageSampler.h"

#include <QTest>

using namespace TagOS::System;

class CpuUsageSamplerTest : public QObject
{
    Q_OBJECT

private slots:
    void firstSampleIsUnknown();
    void computesBusyRatio();
    void clampsToHundred();
    void resetsOnCounterRegression();
    void keepsLastWhenNoJiffiesElapsed();
};

void CpuUsageSamplerTest::firstSampleIsUnknown()
{
    CpuUsageSampler sampler;
    QCOMPARE(sampler.add(CpuTimes{100, 0, 100, 800, 0, 0, 0, 0}), -1.0);
    QVERIFY(sampler.hasSample());
}

void CpuUsageSamplerTest::computesBusyRatio()
{
    CpuUsageSampler sampler;
    CpuTimes first;
    first.idle = 1000;
    first.user = 500;
    sampler.add(first);

    CpuTimes second = first;
    second.idle += 300; // 300 idle jiffies
    second.user += 100; // 100 busy jiffies -> 25%

    QCOMPARE(sampler.add(second), 25.0);
    QCOMPARE(sampler.lastPercent(), 25.0);
}

void CpuUsageSamplerTest::clampsToHundred()
{
    CpuUsageSampler sampler;
    CpuTimes first;
    first.idle = 100;
    sampler.add(first);

    // busy delta wider than total delta can happen when iowait is folded into
    // idle by the kernel; the sampler must never report more than 100%.
    CpuTimes second;
    second.idle = 100;
    second.user = 50;
    QCOMPARE(sampler.add(second), 100.0);
}

void CpuUsageSamplerTest::resetsOnCounterRegression()
{
    CpuUsageSampler sampler;
    CpuTimes high;
    high.idle = 5000;
    high.user = 5000;
    sampler.add(high);

    CpuTimes low;
    low.idle = 10;
    low.user = 10;
    QCOMPARE(sampler.add(low), -1.0);
    // The regressed sample becomes the new baseline.
    low.idle += 50;
    QCOMPARE(sampler.add(low), 0.0);
}

void CpuUsageSamplerTest::keepsLastWhenNoJiffiesElapsed()
{
    CpuUsageSampler sampler;
    CpuTimes first;
    first.idle = 100;
    sampler.add(first);

    CpuTimes second = first;
    second.user = 100;
    QCOMPARE(sampler.add(second), 100.0);

    // Same totals as the previous accepted sample: keep the previous answer.
    QCOMPARE(sampler.add(second), 100.0);
}

QTEST_GUILESS_MAIN(CpuUsageSamplerTest)
#include "tst_cpu_sampler.moc"
