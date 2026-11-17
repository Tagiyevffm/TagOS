#include "tagflow/FlowClient.h"

#include "tag-system/Format.h"
#include "tag-system/SystemProbe.h"

#include <QTimer>
#include <QTimerEvent>

namespace TagOS::Flow {
namespace {
constexpr int kHistoryLimit = 60;
}

FlowClient::FlowClient(QObject *parent)
    : QObject(parent)
    , m_backend(makePreferredBackend())
{
    m_started = true;
    restartTimer();
    refresh();
    // The first reading only primes the CPU delta. Take a second one shortly
    // after so gauges show a number immediately instead of "--" for a full
    // polling interval.
    QTimer::singleShot(150, this, &FlowClient::refresh);
}

FlowClient::~FlowClient()
{
    if (m_timerId != 0)
        killTimer(m_timerId);
}

QString FlowClient::transportId() const
{
    return Flow::transportId(m_backend->transport());
}

QString FlowClient::transportLabel() const
{
    return Flow::transportLabel(m_backend->transport());
}

Transport FlowClient::transportEnum() const
{
    return m_backend->transport();
}

QString FlowClient::tagosVersion() const
{
    return TagOS::System::tagosVersion();
}

void FlowClient::setPollIntervalMs(int intervalMs)
{
    const int bounded = qBound(200, intervalMs, 10000);
    if (bounded == m_pollIntervalMs)
        return;
    m_pollIntervalMs = bounded;
    restartTimer();
    emit pollIntervalMsChanged();
}

void FlowClient::restartTimer()
{
    if (m_timerId != 0)
        killTimer(m_timerId);
    m_timerId = startTimer(m_pollIntervalMs, Qt::CoarseTimer);
}

void FlowClient::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timerId) {
        refresh();
        return;
    }
    QObject::timerEvent(event);
}

void FlowClient::refresh()
{
    if (!m_backend)
        return;

    if (!m_backend->isAvailable()) {
        switchToLocal(tr("Metrics transport %1 became unavailable").arg(m_backend->name()));
        if (!m_backend)
            return;
    }

    TagOS::System::Snapshot snapshot = m_backend->sample();
    if (snapshot.sampledAtMs == 0) {
        ++m_consecutiveFailures;
        if (m_backend->transport() == Transport::Dbus && !m_fellBackToLocal) {
            switchToLocal(tr("No snapshot from %1").arg(m_backend->name()));
            snapshot = m_backend->sample();
        }
        if (snapshot.sampledAtMs == 0) {
            emit errorOccurred(tr("Could not read system metrics"));
            return;
        }
    }
    m_consecutiveFailures = 0;
    applySnapshot(snapshot);
}

void FlowClient::switchToLocal(const QString &reason)
{
    if (m_backend->transport() == Transport::Local) {
        emit errorOccurred(reason);
        return;
    }
    m_backend = makeLocalBackend();
    m_fellBackToLocal = true;
    emit transportChanged();
    emit errorOccurred(reason);
}

void FlowClient::applySnapshot(const TagOS::System::Snapshot &snapshot)
{
    m_snapshot = snapshot;
    m_valid = true;
    appendHistory(snapshot.cpuPercent, snapshot.memory.usedPercent());
    emit snapshotChanged();
    // The first sample only primes the CPU delta; leave it out of the history.
    if (snapshot.cpuPercent >= 0.0)
        emit historyChanged();
}

void FlowClient::appendHistory(double cpu, double memory)
{
    if (cpu >= 0.0) {
        m_cpuHistoryValues.append(cpu);
        while (m_cpuHistoryValues.size() > kHistoryLimit)
            m_cpuHistoryValues.removeFirst();
    }
    if (memory >= 0.0) {
        m_memoryHistoryValues.append(memory);
        while (m_memoryHistoryValues.size() > kHistoryLimit)
            m_memoryHistoryValues.removeFirst();
    }

    m_cpuHistory.clear();
    m_cpuHistory.reserve(m_cpuHistoryValues.size());
    for (double value : m_cpuHistoryValues)
        m_cpuHistory.append(value);

    m_memoryHistory.clear();
    m_memoryHistory.reserve(m_memoryHistoryValues.size());
    for (double value : m_memoryHistoryValues)
        m_memoryHistory.append(value);
}

void FlowClient::clearHistory()
{
    m_cpuHistoryValues.clear();
    m_memoryHistoryValues.clear();
    m_cpuHistory.clear();
    m_memoryHistory.clear();
    emit historyChanged();
}

QString FlowClient::cpuPercentText() const
{
    return TagOS::System::formatPercent(m_snapshot.cpuPercent);
}

QString FlowClient::memoryText() const
{
    return QStringLiteral("%1 / %2")
        .arg(TagOS::System::formatBytes(m_snapshot.memory.usedBytes()),
             TagOS::System::formatBytes(m_snapshot.memory.totalBytes));
}

double FlowClient::swapUsedPercent() const
{
    if (m_snapshot.memory.swapTotalBytes == 0)
        return 0.0;
    return 100.0 * static_cast<double>(m_snapshot.memory.swapUsedBytes())
        / static_cast<double>(m_snapshot.memory.swapTotalBytes);
}

QString FlowClient::diskText() const
{
    return QStringLiteral("%1 / %2")
        .arg(TagOS::System::formatBytes(m_snapshot.disk.usedBytes()),
             TagOS::System::formatBytes(m_snapshot.disk.totalBytes));
}

QString FlowClient::uptimeText() const
{
    return TagOS::System::formatUptime(m_snapshot.uptimeSeconds);
}

QString FlowClient::loadText() const
{
    return TagOS::System::formatLoad(m_snapshot.load);
}

} // namespace TagOS::Flow
