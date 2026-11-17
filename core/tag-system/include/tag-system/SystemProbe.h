#pragma once

#include <QString>

#include "tag-system/Metrics.h"

namespace TagOS::System {

/// Samples the running system using the best available platform backend.
/// The returned CPU percentage is the delta since the previous call.
Snapshot sampleSystem();

/// Version string of this TagOS tree (`0.1.0 (Aurora)`).
QString tagosVersion();

/// "Arch Linux" on a TagOS host, "Windows (dev host)" on a developer machine.
QString platformDisplayName();

} // namespace TagOS::System
