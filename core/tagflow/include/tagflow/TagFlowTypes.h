#pragma once

#include <QString>
#include <QVariantMap>

#include "tagflow/FlowExport.h"

#include "tag-system/Metrics.h"

namespace TagOS::Flow {

/// How this process is currently talking to the system metrics source.
enum class Transport {
    Local,  ///< In-process sampling (Windows dev host, or D-Bus unavailable).
    Dbus,   ///< org.tagos.Flow1 on the session bus, served by tag-flowd.
};

TAGOS_FLOW_EXPORT QString transportId(Transport transport);
TAGOS_FLOW_EXPORT QString transportLabel(Transport transport);

// ---------------------------------------------------------------------------
// Wire format shared by tag-flowd and every client.
// ---------------------------------------------------------------------------
// The canonical contract is dbus/org.tagos.Flow1.xml. Values are sent as
// QVariantMap so that older clients keep working when a new key is added:
// unknown keys are ignored, missing keys fall back to their defaults.

inline constexpr int kSchemaVersion = 1;

TAGOS_FLOW_EXPORT QVariantMap toVariantMap(const TagOS::System::Snapshot &snapshot);
TAGOS_FLOW_EXPORT TagOS::System::Snapshot snapshotFromVariantMap(const QVariantMap &map, bool *ok = nullptr);

} // namespace TagOS::Flow
