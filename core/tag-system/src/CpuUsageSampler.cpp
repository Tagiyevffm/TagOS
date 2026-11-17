#include "tag-system/CpuUsageSampler.h"

#include <QtGlobal>

namespace TagOS::System {

double CpuUsageSampler::add(const CpuTimes &times)
{
    if (!m_hasPrevious) {
        m_previous = times;
        m_hasPrevious = true;
        m_lastPercent = -1.0;
        return m_lastPercent;
    }

    const quint64 previousTotal = m_previous.total();
    const quint64 previousBusy = m_previous.busy();
    m_previous = times;

    // Counters must not regress; if they do, the clock domain changed (suspend,
    // container restart, CPU hotplug) and the delta is meaningless.
    if (times.total() < previousTotal || times.busy() < previousBusy) {
        m_lastPercent = -1.0;
        return m_lastPercent;
    }

    const quint64 deltaTotal = times.total() - previousTotal;
    if (deltaTotal == 0) {
        // No jiffies elapsed yet: keep the previous answer instead of flapping.
        return m_lastPercent;
    }

    const quint64 deltaBusy = times.busy() - previousBusy;
    // Guard against a busy delta wider than the total delta (possible when the
    // kernel folds iowait into idle); clamp instead of reporting > 100%.
    const double ratio = static_cast<double>(qMin(deltaBusy, deltaTotal)) / static_cast<double>(deltaTotal);
    m_lastPercent = qBound(0.0, ratio * 100.0, 100.0);
    return m_lastPercent;
}

void CpuUsageSampler::reset()
{
    m_previous = CpuTimes{};
    m_hasPrevious = false;
    m_lastPercent = -1.0;
}

} // namespace TagOS::System
