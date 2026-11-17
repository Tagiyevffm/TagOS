#include "tagflow/Backend.h"

#include "tag-system/SystemProbe.h"

namespace TagOS::Flow {
namespace {

/// Samples the running kernel directly. Always available, always cheap.
class LocalBackend final : public Backend
{
public:
    Transport transport() const override { return Transport::Local; }
    QString name() const override { return QStringLiteral("local"); }
    bool isAvailable() const override { return true; }
    TagOS::System::Snapshot sample() override { return TagOS::System::sampleSystem(); }
};

} // namespace

std::unique_ptr<Backend> makeLocalBackend()
{
    return std::make_unique<LocalBackend>();
}

std::unique_ptr<Backend> makePreferredBackend()
{
#ifdef TAGOS_HAS_DBUS
    if (auto dbus = makeDbusBackend()) {
        if (dbus->isAvailable())
            return dbus;
    }
#endif
    // No tag-flowd on this bus (or no D-Bus at all): sample in-process so the
    // desktop keeps working instead of showing empty gauges.
    return makeLocalBackend();
}

} // namespace TagOS::Flow
