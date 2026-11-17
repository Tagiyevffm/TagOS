#include "tagflow/Backend.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariantMap>

namespace TagOS::Flow {
namespace {

constexpr auto kService = "org.tagos.Flow";
constexpr auto kPath = "/org/tagos/Flow";
constexpr auto kInterface = "org.tagos.Flow1";

/// Talks to tag-flowd over the session bus.
///
/// Failures are surfaced through `isAvailable()`; FlowClient reacts by falling
/// back to local sampling so a crashed service never blanks the desktop.
class DbusBackend final : public Backend
{
public:
    DbusBackend()
        : m_interface(kService, kPath, kInterface, QDBusConnection::sessionBus())
    {
        m_available = QDBusConnection::sessionBus().isConnected() && m_interface.isValid();
        if (m_available) {
            const QDBusReply<bool> reply = m_interface.call(QStringLiteral("Ping"));
            m_available = reply.isValid() && reply.value();
        }
    }

    Transport transport() const override { return Transport::Dbus; }
    QString name() const override { return QStringLiteral("org.tagos.Flow1"); }
    bool isAvailable() const override { return m_available; }

    TagOS::System::Snapshot sample() override
    {
        if (!m_available)
            return {};
        const QDBusReply<QVariantMap> reply = m_interface.call(QStringLiteral("Snapshot"));
        if (!reply.isValid()) {
            m_available = false;
            return {};
        }
        bool ok = false;
        TagOS::System::Snapshot snapshot = snapshotFromVariantMap(reply.value(), &ok);
        if (!ok) {
            m_available = false;
            return {};
        }
        return snapshot;
    }

private:
    QDBusInterface m_interface;
    bool m_available = false;
};

} // namespace

std::unique_ptr<Backend> makeDbusBackend()
{
    return std::make_unique<DbusBackend>();
}

} // namespace TagOS::Flow
