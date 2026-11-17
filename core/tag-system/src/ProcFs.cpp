#include "tag-system/ProcFs.h"

#include <QFile>
#include <QStringList>

namespace TagOS::System {

ProcFs::ProcFs(QString root)
    : m_root(std::move(root))
{
}

QString ProcFs::readFile(const QString &name, bool *ok) const
{
    QFile file(m_root + QLatin1Char('/') + name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok)
            *ok = false;
        return {};
    }
    if (ok)
        *ok = true;
    return QString::fromUtf8(file.readAll());
}

CpuTimes ProcFs::cpuTimes(bool *ok) const
{
    const QString text = readFile(QStringLiteral("stat"), ok);
    if (text.isEmpty())
        return {};

    // First row: "cpu  user nice system idle iowait irq softirq steal guest guest_nice"
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (!line.startsWith(QStringLiteral("cpu")))
            continue;
        const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 5 || fields.first() != QStringLiteral("cpu"))
            continue;

        // Fields are 100 Hz jiffies; only ratios matter, so keep the raw counts.
        QVarLengthArray<quint64, 10> values;
        for (qsizetype i = 1; i < fields.size(); ++i) {
            bool converted = false;
            const quint64 value = fields.at(i).toULongLong(&converted);
            values.append(converted ? value : 0);
        }
        auto at = [&values](qsizetype index) { return index < values.size() ? values.at(index) : 0; };

        CpuTimes times;
        times.user = at(0);
        times.nice = at(1);
        times.system = at(2);
        times.idle = at(3);
        times.iowait = at(4);
        times.irq = at(5);
        times.softirq = at(6);
        times.steal = at(7);
        times.guest = at(8);
        times.guestNice = at(9);
        return times;
    }

    if (ok)
        *ok = false;
    return {};
}

MemoryInfo ProcFs::memoryInfo(bool *ok) const
{
    const QString text = readFile(QStringLiteral("meminfo"), ok);
    if (text.isEmpty())
        return {};

    MemoryInfo info;
    qint64 totalKb = -1;
    qint64 availableKb = -1;
    qint64 freeKb = -1;
    qint64 buffersKb = 0;
    qint64 cachedKb = 0;
    qint64 swapTotalKb = 0;
    qint64 swapFreeKb = 0;

    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        const QString key = line.left(colon).trimmed();
        const QString rest = line.mid(colon + 1).trimmed();
        // "MemTotal:       16316136 kB"
        const QStringList parts = rest.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        bool converted = false;
        const qint64 valueKb = parts.isEmpty() ? -1 : parts.first().toLongLong(&converted);
        if (!converted)
            continue;

        if (key == QLatin1String("MemTotal"))
            totalKb = valueKb;
        else if (key == QLatin1String("MemAvailable"))
            availableKb = valueKb;
        else if (key == QLatin1String("MemFree"))
            freeKb = valueKb;
        else if (key == QLatin1String("Buffers"))
            buffersKb = valueKb;
        else if (key == QLatin1String("Cached"))
            cachedKb = valueKb;
        else if (key == QLatin1String("SwapTotal"))
            swapTotalKb = valueKb;
        else if (key == QLatin1String("SwapFree"))
            swapFreeKb = valueKb;
    }

    if (totalKb <= 0) {
        if (ok)
            *ok = false;
        return {};
    }

    // Kernels before 3.14 do not export MemAvailable; approximate it the way
    // `free` does (free + buffers + cached) so old targets keep working.
    if (availableKb < 0)
        availableKb = qMax<qint64>(0, freeKb) + buffersKb + cachedKb;

    constexpr qint64 bytesPerKb = 1024;
    info.totalBytes = static_cast<quint64>(totalKb) * bytesPerKb;
    info.availableBytes = static_cast<quint64>(qBound<qint64>(0, availableKb, totalKb)) * bytesPerKb;
    info.swapTotalBytes = static_cast<quint64>(qMax<qint64>(0, swapTotalKb)) * bytesPerKb;
    info.swapFreeBytes = static_cast<quint64>(qMax<qint64>(0, swapFreeKb)) * bytesPerKb;
    return info;
}

LoadAverage ProcFs::loadAverage(bool *ok) const
{
    const QString text = readFile(QStringLiteral("loadavg"), ok);
    if (text.isEmpty())
        return {};

    const QStringList fields = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 3) {
        if (ok)
            *ok = false;
        return {};
    }

    bool okOne = false;
    bool okFive = false;
    bool okFifteen = false;
    LoadAverage load;
    load.one = fields.at(0).toDouble(&okOne);
    load.five = fields.at(1).toDouble(&okFive);
    load.fifteen = fields.at(2).toDouble(&okFifteen);
    load.available = okOne && okFive && okFifteen;
    if (!load.available && ok)
        *ok = false;
    return load;
}

qint64 ProcFs::uptimeSeconds(bool *ok) const
{
    const QString text = readFile(QStringLiteral("uptime"), ok);
    if (text.isEmpty())
        return -1;

    const QStringList fields = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    bool converted = false;
    const double seconds = fields.isEmpty() ? -1.0 : fields.first().toDouble(&converted);
    if (!converted || seconds < 0) {
        if (ok)
            *ok = false;
        return -1;
    }
    return static_cast<qint64>(seconds);
}

} // namespace TagOS::System
