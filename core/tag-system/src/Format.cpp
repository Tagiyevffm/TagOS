#include "tag-system/Format.h"

#include "tag-system/Metrics.h"

#include <QStringList>
#include <QtMath>

namespace TagOS::System {
namespace {

/// "1.50" -> "1.5", "100.00" -> "100": keeps panels tidy without losing
/// meaningful decimals.
QString trimTrailingZeros(QString number)
{
    if (!number.contains(QLatin1Char('.')))
        return number;
    while (number.endsWith(QLatin1Char('0')))
        number.chop(1);
    if (number.endsWith(QLatin1Char('.')))
        number.chop(1);
    return number;
}

} // namespace

QString formatBytes(quint64 bytes, int decimals)
{
    static const QStringList units = {QStringLiteral("B"),  QStringLiteral("KiB"), QStringLiteral("MiB"),
                                      QStringLiteral("GiB"), QStringLiteral("TiB"), QStringLiteral("PiB")};

    if (bytes == 0)
        return QStringLiteral("0 B");

    double value = static_cast<double>(bytes);
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < units.size() - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    const int effectiveDecimals = unitIndex == 0 ? 0 : qBound(0, decimals, 3);
    const QString number = trimTrailingZeros(QString::number(value, 'f', effectiveDecimals));
    return number + QLatin1Char(' ') + units.at(unitIndex);
}

QString formatUptime(qint64 seconds)
{
    if (seconds < 0)
        return QStringLiteral("--");

    const qint64 days = seconds / 86400;
    const qint64 hours = (seconds % 86400) / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    const qint64 secs = seconds % 60;

    auto pad = [](qint64 value) { return QString::number(value).rightJustified(2, QLatin1Char('0')); };

    if (days > 0)
        return QStringLiteral("%1d %2:%3").arg(days).arg(hours).arg(pad(minutes));
    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours).arg(pad(minutes), pad(secs));
    return QStringLiteral("%1:%2").arg(minutes).arg(pad(secs));
}

QString formatPercent(double percent, int decimals)
{
    if (percent < 0.0)
        return QStringLiteral("--");
    const int effectiveDecimals = qBound(0, decimals, 2);
    return trimTrailingZeros(QString::number(percent, 'f', effectiveDecimals)) + QLatin1Char('%');
}

QString formatLoad(const LoadAverage &load)
{
    if (!load.available)
        return QStringLiteral("--");
    return QStringLiteral("%1 %2 %3")
        .arg(load.one, 0, 'f', 2)
        .arg(load.five, 0, 'f', 2)
        .arg(load.fifteen, 0, 'f', 2);
}

} // namespace TagOS::System
