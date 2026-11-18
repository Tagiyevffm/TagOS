#include "tag-system/Monitors.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSysInfo>
#include <QThread>

#ifdef Q_OS_UNIX
#include <csignal>
#include <signal.h>
#endif

#include "tag-system/Format.h"

namespace TagOS::System {
namespace {

QString readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

QString formatRate(double bytesPerSecond)
{
    if (bytesPerSecond < 1.0)
        return QStringLiteral("0 B/s");
    return formatBytes(static_cast<quint64>(bytesPerSecond)) + QStringLiteral("/s");
}

constexpr qint64 kPageSize = 4096;

} // namespace

// ---------------------------------------------------------------------------
// ProcessList
// ---------------------------------------------------------------------------

ProcessList::ProcessList(QObject *parent, const QString &procRoot)
    : QAbstractListModel(parent)
    , m_root(procRoot)
{
    m_available = QFileInfo::exists(m_root + QStringLiteral("/stat"));
    m_cpuCount = std::max(1, QThread::idealThreadCount());
    m_memoryTotal = totalMemoryBytes();
    refresh();
}

int ProcessList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ProcessList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows.at(index.row());

    switch (role) {
    case PidRole:
        return row.pid;
    case NameRole:
        return row.name;
    case CpuRole:
        return row.cpuPercent;
    case CpuTextRole:
        return QString::number(row.cpuPercent, 'f', 1) + QLatin1Char('%');
    case MemoryRole:
        return row.rssBytes;
    case MemoryTextRole:
        return formatBytes(static_cast<quint64>(row.rssBytes));
    case StateRole:
        return row.state;
    case CommandRole:
        return row.command;
    default:
        return {};
    }
}

QHash<int, QByteArray> ProcessList::roleNames() const
{
    return {
        {PidRole, "pid"},        {NameRole, "name"},
        {CpuRole, "cpu"},        {CpuTextRole, "cpuText"},
        {MemoryRole, "memory"},  {MemoryTextRole, "memoryText"},
        {StateRole, "state"},    {CommandRole, "command"},
    };
}

QString ProcessList::summary() const
{
    if (!m_available)
        return tr("/proc okunamıyor");
    return tr("%1 süreç · %2 iş parçacığı").arg(m_processCount).arg(m_threadCount);
}

qint64 ProcessList::totalMemoryBytes() const
{
    const QString meminfo = readAll(m_root + QStringLiteral("/meminfo"));
    for (const QString &line : meminfo.split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("MemTotal:"))) {
            const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2)
                return parts.at(1).toLongLong() * 1024;
        }
    }
    return 0;
}

bool ProcessList::readCpuTotal(quint64 *total) const
{
    const QString stat = readAll(m_root + QStringLiteral("/stat"));
    if (stat.isEmpty())
        return false;

    const QString firstLine = stat.section(QLatin1Char('\n'), 0, 0);
    const QStringList fields = firstLine.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 5 || fields.first() != QLatin1String("cpu"))
        return false;

    quint64 sum = 0;
    for (int i = 1; i < fields.size(); ++i) {
        bool ok = false;
        const quint64 value = fields.at(i).toULongLong(&ok);
        if (ok)
            sum += value;
    }
    *total = sum;
    return true;
}

bool ProcessList::readProcess(int pid, Row *row, quint64 *totalTicks, quint64 *rssPages) const
{
    *totalTicks = 0;
    *rssPages = 0;

    const QString statPath = m_root + QStringLiteral("/%1/stat").arg(pid);
    const QString stat = readAll(statPath);
    if (stat.isEmpty())
        return false;

    // The command name may contain spaces and is wrapped in parentheses.
    const int open = stat.indexOf(QLatin1Char('('));
    const int close = stat.lastIndexOf(QLatin1Char(')'));
    if (open < 0 || close < open)
        return false;

    row->pid = pid;
    row->name = stat.mid(open + 1, close - open - 1);

    const QStringList rest = stat.mid(close + 2).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    // rest[0] = state, rest[11] = utime, rest[12] = stime, rest[21] = rss (pages)
    if (rest.size() < 22)
        return false;
    row->state = rest.at(0);
    const quint64 utime = rest.at(11).toULongLong();
    const quint64 stime = rest.at(12).toULongLong();
    *totalTicks = utime + stime;
    *rssPages = rest.at(21).toULongLong();

    const QString cmdline = readAll(m_root + QStringLiteral("/%1/cmdline").arg(pid));
    row->command = cmdline;
    row->command.replace(QChar(0), QLatin1Char(' '));
    row->command = row->command.trimmed();
    if (row->command.isEmpty())
        row->command = row->name;

    return true;
}

void ProcessList::refresh()
{
    if (!m_available)
        return;

    quint64 totalNow = 0;
    if (!readCpuTotal(&totalNow)) {
        emit refreshFailed(tr("/proc/stat okunamadı"));
        return;
    }

    const quint64 totalDelta = m_previousTotal > 0 && totalNow > m_previousTotal ? totalNow - m_previousTotal : 0;

    const QDir procDir(m_root);
    const QStringList entries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QList<Row> rows;
    QHash<int, quint64> ticksNow;
    int threads = 0;
    int processes = 0;

    for (const QString &entry : entries) {
        bool ok = false;
        const int pid = entry.toInt(&ok);
        if (!ok)
            continue;

        Row row;
        quint64 ticks = 0;
        quint64 rssPages = 0;
        if (!readProcess(pid, &row, &ticks, &rssPages))
            continue;

        ++processes;
        threads += 1;
        ticksNow.insert(pid, ticks);

        const quint64 previous = m_previousTicks.value(pid, 0);
        if (totalDelta > 0 && previous > 0 && ticks >= previous) {
            const double share = static_cast<double>(ticks - previous) / static_cast<double>(totalDelta);
            row.cpuPercent = qBound(0.0, share * 100.0 * m_cpuCount, 100.0 * m_cpuCount);
        }
        row.rssBytes = static_cast<qint64>(rssPages) * kPageSize;
        row.memoryPercent = m_memoryTotal > 0
            ? qBound(0.0, 100.0 * row.rssBytes / m_memoryTotal, 100.0)
            : 0.0;
        rows.append(row);
    }

    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        if (a.cpuPercent != b.cpuPercent)
            return a.cpuPercent > b.cpuPercent;
        return a.rssBytes > b.rssBytes;
    });

    beginResetModel();
    m_rows = rows;
    m_previousTicks = ticksNow;
    m_previousTotal = totalNow;
    m_processCount = processes;
    m_threadCount = threads;
    endResetModel();
    emit countChanged();
}

QVariantMap ProcessList::at(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    const Row &entry = m_rows.at(row);
    return {
        {QStringLiteral("pid"), entry.pid},
        {QStringLiteral("name"), entry.name},
        {QStringLiteral("command"), entry.command},
        {QStringLiteral("cpu"), entry.cpuPercent},
        {QStringLiteral("cpuText"), QString::number(entry.cpuPercent, 'f', 1) + QLatin1Char('%')},
        {QStringLiteral("memory"), entry.rssBytes},
        {QStringLiteral("memoryText"), formatBytes(static_cast<quint64>(entry.rssBytes))},
    };
}

bool ProcessList::kill(int pid)
{
    if (pid <= 1) {
        emit refreshFailed(tr("Bu süreç sonlandırılamaz"));
        return false;
    }
#ifdef Q_OS_UNIX
    if (::kill(pid, SIGTERM) != 0) {
        emit refreshFailed(tr("Süreç sonlandırılamadı (pid %1)").arg(pid));
        return false;
    }
    return true;
#else
    Q_UNUSED(pid)
    emit refreshFailed(tr("Süreç sonlandırma bu platformda desteklenmiyor"));
    return false;
#endif
}

// ---------------------------------------------------------------------------
// NetworkSampler
// ---------------------------------------------------------------------------

NetworkSampler::NetworkSampler(QObject *parent, const QString &procRoot)
    : QObject(parent)
    , m_root(procRoot)
{
    m_clock.start();
    refresh();
}

QString NetworkSampler::rxRateText() const
{
    return m_available ? formatRate(m_rxRate) : QStringLiteral("--");
}

QString NetworkSampler::txRateText() const
{
    return m_available ? formatRate(m_txRate) : QStringLiteral("--");
}

QString NetworkSampler::rxTotalText() const
{
    return m_available ? formatBytes(m_rxBytes) : QStringLiteral("--");
}

QString NetworkSampler::txTotalText() const
{
    return m_available ? formatBytes(m_txBytes) : QStringLiteral("--");
}

void NetworkSampler::refresh()
{
    const QString dev = readAll(m_root + QStringLiteral("/net/dev"));
    if (dev.isEmpty()) {
        if (m_available) {
            m_available = false;
            emit statsChanged();
        }
        return;
    }

    quint64 rx = 0;
    quint64 tx = 0;
    QString selected;

    for (const QString &line : dev.split(QLatin1Char('\n'))) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0)
            continue;
        const QString name = line.left(colon).trimmed();
        if (name == QLatin1String("lo"))
            continue;

        const QStringList fields = line.mid(colon + 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 9)
            continue;

        const quint64 ifRx = fields.at(0).toULongLong();
        const quint64 ifTx = fields.at(8).toULongLong();
        if (ifRx == 0 && ifTx == 0)
            continue;
        if (selected.isEmpty() || (ifRx + ifTx) > (rx + tx)) {
            selected = name;
            rx = ifRx;
            tx = ifTx;
        }
    }

    if (selected.isEmpty()) {
        // No active interface: still report totals as zero rather than failing.
        m_available = true;
        m_interface.clear();
        m_rxBytes = 0;
        m_txBytes = 0;
        emit statsChanged();
        return;
    }

    const double seconds = m_clock.restart() / 1000.0;
    if (m_hasSample && seconds > 0.2) {
        if (rx >= m_previousRx && m_interface == selected)
            m_rxRate = (rx - m_previousRx) / seconds;
        if (tx >= m_previousTx && m_interface == selected)
            m_txRate = (tx - m_previousTx) / seconds;
    }

    m_hasSample = true;
    m_available = true;
    m_interface = selected;
    m_rxBytes = rx;
    m_txBytes = tx;
    m_previousRx = rx;
    m_previousTx = tx;
    emit statsChanged();
}

// ---------------------------------------------------------------------------
// ThermalReader
// ---------------------------------------------------------------------------

ThermalReader::ThermalReader(QObject *parent, const QString &thermalRoot)
    : QObject(parent)
    , m_root(thermalRoot)
{
    refresh();
}

QString ThermalReader::text() const
{
    if (!m_available)
        return QStringLiteral("--");
    return QString::number(m_celsius, 'f', 0) + QStringLiteral(" °C");
}

void ThermalReader::refresh()
{
    QDir dir(m_root);
    if (!dir.exists()) {
        if (m_available) {
            m_available = false;
            emit changed();
        }
        return;
    }

    double best = -1.0;
    QString bestLabel;
    const QStringList zones = dir.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs);
    for (const QString &zone : zones) {
        const QString tempText = readAll(dir.filePath(zone + QStringLiteral("/temp"))).trimmed();
        bool ok = false;
        double value = tempText.toDouble(&ok);
        if (!ok)
            continue;
        while (value > 1000.0)   // some drivers report millidegrees
            value /= 1000.0;
        if (value > best) {
            best = value;
            bestLabel = readAll(dir.filePath(zone + QStringLiteral("/type"))).trimmed();
        }
    }

    if (best < 0) {
        if (m_available) {
            m_available = false;
            emit changed();
        }
        return;
    }

    m_available = true;
    m_celsius = best;
    m_label = bestLabel;
    emit changed();
}

} // namespace TagOS::System
