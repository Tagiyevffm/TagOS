#pragma once

#include <QString>

#include "tag-system/Metrics.h"

namespace TagOS::System {

/// Human readable formatting helpers shared by every TagOS surface.
/// Kept pure (no locale, no UI) so that panels, CLI tools and tests agree.
QString formatBytes(quint64 bytes, int decimals = 1);
QString formatUptime(qint64 seconds);
QString formatPercent(double percent, int decimals = 0);
QString formatLoad(const LoadAverage &load);

} // namespace TagOS::System
