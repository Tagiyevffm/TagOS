#include "tag-system/PowerManager.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace TagOS::System {

class PowerManagerPrivate
{
public:
    QString lastError;
    bool dryRun = false;
    QStringList performedActions;
};

PowerManager::PowerManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<PowerManagerPrivate>())
{
    d->dryRun = QProcessEnvironment::systemEnvironment().value(QStringLiteral("TAGOS_DRY_RUN")) == QLatin1String("1");
}

PowerManager::~PowerManager() = default;

bool PowerManager::isAvailable() const
{
#ifdef Q_OS_LINUX
    if (d->dryRun)
        return true;
    return !QStandardPaths::findExecutable(QStringLiteral("systemctl")).isEmpty();
#else
    return d->dryRun; // No power actions on a developer host.
#endif
}

bool PowerManager::isDryRun() const
{
    return d->dryRun;
}

QString PowerManager::lastError() const
{
    return d->lastError;
}

bool PowerManager::perform(const QString &action, const QString &program, const QStringList &arguments)
{
    emit actionRequested(action);

    if (d->dryRun) {
        d->performedActions.append(action);
        return true;
    }

#ifndef Q_OS_LINUX
    d->lastError = tr("Power actions are only available in a TagOS session (%1)").arg(action);
    emit lastErrorChanged();
    return false;
#else
    bool started = QProcess::startDetached(program, arguments);
    if (!started) {
        d->lastError = tr("Could not run %1").arg(program);
        emit lastErrorChanged();
    }
    return started;
#endif
}

bool PowerManager::reboot()
{
    return perform(QStringLiteral("reboot"), QStringLiteral("systemctl"), {QStringLiteral("reboot")});
}

bool PowerManager::powerOff()
{
    return perform(QStringLiteral("poweroff"), QStringLiteral("systemctl"), {QStringLiteral("poweroff")});
}

bool PowerManager::suspend()
{
    return perform(QStringLiteral("suspend"), QStringLiteral("systemctl"), {QStringLiteral("suspend")});
}

bool PowerManager::logOut()
{
    return perform(QStringLiteral("logout"),
                   QStringLiteral("loginctl"),
                   {QStringLiteral("terminate-user"), qEnvironmentVariable("USER")});
}

} // namespace TagOS::System
