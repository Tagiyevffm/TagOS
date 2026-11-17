#pragma once

#include "tag-system/Metrics.h"

namespace TagOS::System {

/// Turns consecutive `CpuTimes` readings into a busy percentage.
///
/// Kept as a tiny value type (instead of hiding the previous sample inside the
/// platform probe) so the arithmetic is deterministic and unit tested.
class CpuUsageSampler
{
public:
    /// Feeds a new reading and returns the busy percentage in `[0, 100]`.
    /// Returns -1 on the first call, and resets on counter regressions
    /// (suspend/resume, container restart, wrapping counters).
    double add(const CpuTimes &times);

    void reset();
    bool hasSample() const { return m_hasPrevious; }
    const CpuTimes &previous() const { return m_previous; }
    double lastPercent() const { return m_lastPercent; }

private:
    CpuTimes m_previous;
    bool m_hasPrevious = false;
    double m_lastPercent = -1.0;
};

} // namespace TagOS::System
