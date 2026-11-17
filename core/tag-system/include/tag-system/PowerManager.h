#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QString>

#include <memory>

namespace TagOS::System {

class PowerManagerPrivate;

/// Session power actions.
///
/// TagOS never talks to the hardware directly: it asks logind through
/// `systemctl`/`loginctl`, which keeps polkit in charge of authorisation.
/// `TAGOS_DRY_RUN=1` turns every action into a recorded no-op, which is what
/// the automated desktop tests use.
class PowerManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(bool dryRun READ isDryRun CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit PowerManager(QObject *parent = nullptr);
    ~PowerManager() override;

    bool isAvailable() const;
    bool isDryRun() const;
    QString lastError() const;

    /// Actions return false and set `lastError` when the request was rejected.
    Q_INVOKABLE bool reboot();
    Q_INVOKABLE bool powerOff();
    Q_INVOKABLE bool suspend();
    Q_INVOKABLE bool logOut();

signals:
    void lastErrorChanged();
    void actionRequested(const QString &action);

private:
    bool perform(const QString &action, const QString &program, const QStringList &arguments);

    std::unique_ptr<PowerManagerPrivate> d;
};

} // namespace TagOS::System
