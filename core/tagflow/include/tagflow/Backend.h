#pragma once

#include <memory>

#include "tag-system/Metrics.h"
#include "tagflow/FlowExport.h"
#include "tagflow/TagFlowTypes.h"

namespace TagOS::Flow {

/// A metrics transport. Clients never know whether the reading came from D-Bus
/// or from an in-process sample; only `transport()` differs.
class TAGOS_FLOW_EXPORT Backend
{
public:
    virtual ~Backend() = default;

    virtual Transport transport() const = 0;
    virtual QString name() const = 0;
    /// False when the transport is gone (service not registered, bus dropped).
    virtual bool isAvailable() const = 0;
    /// One reading. `sampledAtMs == 0` means "nothing usable".
    virtual TagOS::System::Snapshot sample() = 0;
};

TAGOS_FLOW_EXPORT std::unique_ptr<Backend> makeLocalBackend();
/// Returns nullptr when QtDBus is not part of this build (Windows dev hosts).
TAGOS_FLOW_EXPORT std::unique_ptr<Backend> makeDbusBackend();
/// D-Bus when a tag-flowd is reachable, otherwise in-process sampling.
TAGOS_FLOW_EXPORT std::unique_ptr<Backend> makePreferredBackend();

} // namespace TagOS::Flow
