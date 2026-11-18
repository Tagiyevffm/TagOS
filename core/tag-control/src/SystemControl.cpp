#include "tag-control/SystemControl.h"

#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include "tag-control/ControlParsers.h"

namespace TagOS::Control {
namespace {

bool hasTool(const QString &program)
{
    return !QStandardPaths::findExecutable(program).isEmpty();
}

/// Upper bound for a probe. bluetoothctl waits for a daemon that may not run.
constexpr int kProbeTimeoutMs = 3000;

/// One-shot command execution; the callback is dropped if the manager is gone.
void runProcess(const QString &program, const QStringList &args,
                QObject *context, std::function<void(int, const QString &)> done)
{
    auto *process = new QProcess(context);
    process->setProgram(program);
    process->setArguments(args);
    process->setProcessChannelMode(QProcess::SeparateChannels);

    QPointer<QProcess> guard(process);
    QObject::connect(process, &QProcess::errorOccurred, process, [done, guard](QProcess::ProcessError) {
        if (guard)
            done(-1, QString());
    });
    // Some probes (bluetoothctl with no daemon running) never exit on their own.
    // Without a deadline every refresh adds another process that waits forever.
    QTimer::singleShot(kProbeTimeoutMs, process, [guard, done]() {
        if (!guard || guard->state() == QProcess::NotRunning)
            return;
        guard->kill();
        done(-1, QString());
    });
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), process,
                     [done, guard](int code, QProcess::ExitStatus status) {
                         if (!guard)
                             return;
                         const QString out = QString::fromUtf8(guard->readAllStandardOutput());
                         const QString err = QString::fromUtf8(guard->readAllStandardError());
                         done(status == QProcess::NormalExit ? code : -1, out + err);
                     });
    QObject::connect(process, &QProcess::finished, process, &QObject::deleteLater);
    process->start();
}

QString join(const QStringList &args)
{
    return args.join(QLatin1Char(' '));
}

} // namespace

class SystemControl::Private
{
public:
    QProcess *nightLight = nullptr;
};

SystemControl::SystemControl(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    // Detect once; the availability flags never change during a session.
    m_detectedTools.clear();
    for (const QString &tool : {QStringLiteral("nmcli"), QStringLiteral("bluetoothctl"),
                                QStringLiteral("wpctl"), QStringLiteral("brightnessctl"),
                                QStringLiteral("upower"), QStringLiteral("powerprofilesctl"),
                                QStringLiteral("rfkill"), QStringLiteral("gammastep")}) {
        if (hasTool(tool))
            m_detectedTools.append(tool);
    }
    m_wifiAvailable = m_detectedTools.contains(QStringLiteral("nmcli"));
    m_bluetoothAvailable = m_detectedTools.contains(QStringLiteral("bluetoothctl"));
    m_audioAvailable = m_detectedTools.contains(QStringLiteral("wpctl"));
    m_brightnessAvailable = m_detectedTools.contains(QStringLiteral("brightnessctl"));
    m_batteryAvailable = m_detectedTools.contains(QStringLiteral("upower"));
    m_powerProfilesAvailable = m_detectedTools.contains(QStringLiteral("powerprofilesctl"));
    m_airplaneAvailable = m_detectedTools.contains(QStringLiteral("rfkill"));
    m_nightLightAvailable = m_detectedTools.contains(QStringLiteral("gammastep"));
    refresh();
}

SystemControl::~SystemControl()
{
    if (d && d->nightLight) {
        d->nightLight->disconnect(this);
        d->nightLight->terminate();
        d->nightLight->waitForFinished(500);
        delete d->nightLight;
        d->nightLight = nullptr;
    }
    delete d;
}

QString SystemControl::transportSummary() const
{
    if (m_detectedTools.isEmpty())
        return tr("Sistem servisleri bulunamadı");
    return tr("Backend: %1").arg(m_detectedTools.join(QStringLiteral(", ")));
}

QVariantList SystemControl::capabilities() const
{
    auto row = [](const QString &id, const QString &label, bool available, const QString &detail) {
        return QVariantMap{{QStringLiteral("id"), id},
                           {QStringLiteral("label"), label},
                           {QStringLiteral("available"), available},
                           {QStringLiteral("detail"), detail}};
    };
    return {
        row(QStringLiteral("wifi"), tr("Kablosuz ağ"), m_wifiAvailable,
            m_wifiAvailable ? (m_wifiConnected ? m_wifiSsid : tr("Bağlı değil")) : tr("nmcli yok")),
        row(QStringLiteral("bluetooth"), tr("Bluetooth"), m_bluetoothAvailable,
            m_bluetoothAvailable ? (m_bluetoothEnabled ? tr("Açık") : tr("Kapalı")) : tr("bluetoothctl yok")),
        row(QStringLiteral("audio"), tr("Ses"), m_audioAvailable,
            m_audioAvailable ? tr("%1%").arg(m_volume) : tr("wpctl yok")),
        row(QStringLiteral("brightness"), tr("Parlaklık"), m_brightnessAvailable,
            m_brightnessAvailable ? tr("%1%").arg(m_brightness) : tr("brightnessctl yok")),
        row(QStringLiteral("battery"), tr("Pil"), m_batteryAvailable,
            m_batteryAvailable ? tr("%1%").arg(m_batteryPercent) : tr("upower yok")),
        row(QStringLiteral("power"), tr("Güç profili"), m_powerProfilesAvailable,
            m_powerProfilesAvailable ? m_powerProfile : tr("powerprofilesctl yok")),
        row(QStringLiteral("nightlight"), tr("Gece ışığı"), m_nightLightAvailable,
            m_nightLightAvailable ? (m_nightLightEnabled ? tr("Açık") : tr("Kapalı")) : tr("gammastep yok")),
        row(QStringLiteral("airplane"), tr("Uçak modu"), m_airplaneAvailable,
            m_airplaneAvailable ? (m_airplaneEnabled ? tr("Açık") : tr("Kapalı")) : tr("rfkill yok")),
    };
}

void SystemControl::setError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

void SystemControl::run(const QString &program, const QStringList &args,
                        std::function<void(int, const QString &)> done)
{
    runProcess(program, args, this, [this, done](int code, const QString &output) {
        Q_UNUSED(this)
        done(code, output);
    });
}

void SystemControl::refresh()
{
    if (m_refreshing)
        return;
    m_refreshing = true;
    emit changed();

    probeWifi();
    probeBluetooth();
    probeAudio();
    probeBrightness();
    probeBattery();
    probePowerProfiles();
    probeRfkill();

    // The probes are independent; the flag drops on the next event loop turn so
    // QML can show a spinner without waiting for every tool.
    QTimer::singleShot(0, this, [this] {
        m_refreshing = false;
        emit changed();
    });
}

void SystemControl::probeWifi()
{
    if (!m_wifiAvailable) {
        emit changed();
        return;
    }

    run(QStringLiteral("nmcli"), {QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("WIFI"),
                                  QStringLiteral("radio")},
        [this](int code, const QString &out) {
            if (code != 0) {
                setError(tr("nmcli çalıştırılamadı"));
                return;
            }
            bool ok = false;
            m_wifiEnabled = parseNmcliRadio(out, &ok);
            if (!ok)
                m_wifiAvailable = false;
            emit changed();
        });

    if (!m_wifiEnabled)
        return;

    run(QStringLiteral("nmcli"),
        {QStringLiteral("-t"), QStringLiteral("-f"),
         QStringLiteral("IN-USE,SSID,SIGNAL,SECURITY"), QStringLiteral("device"), QStringLiteral("wifi"),
         QStringLiteral("list"), QStringLiteral("--rescan"), QStringLiteral("no")},
        [this](int code, const QString &out) {
            if (code != 0)
                return;
            const WifiState state = parseNmcliWifiList(out);
            m_wifiConnected = state.connected;
            m_wifiSsid = state.ssid;
            m_wifiStrength = state.strength;
            m_wifiSecurity = state.security;

            QStringList networks;
            for (const QString &line : out.split(QLatin1Char('\n'))) {
                const QStringList fields = line.trimmed().split(QLatin1Char(':'));
                const QString ssid = fields.value(1).trimmed();
                if (!ssid.isEmpty() && !networks.contains(ssid))
                    networks.append(ssid);
            }
            m_wifiNetworks = networks;
            emit changed();
        });
}

void SystemControl::probeBluetooth()
{
    if (!m_bluetoothAvailable)
        return;
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("show")}, [this](int code, const QString &out) {
        if (code != 0 && out.isEmpty())
            return;
        bool ok = false;
        m_bluetoothEnabled = parseBluetoothPowered(out, &ok);
        if (!ok)
            m_bluetoothAvailable = false;
        emit changed();
    });
}

void SystemControl::probeAudio()
{
    if (!m_audioAvailable || m_audioDirty)
        return;
    run(QStringLiteral("wpctl"),
        {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@")},
        [this](int code, const QString &out) {
            if (code != 0) {
                m_audioAvailable = false;
                emit changed();
                return;
            }
            const AudioState state = parseWpctlVolume(out);
            if (!state.valid)
                return;
            m_volume = state.volume;
            m_muted = state.muted;
            emit changed();
        });
}

void SystemControl::probeBrightness()
{
    if (!m_brightnessAvailable)
        return;
    run(QStringLiteral("brightnessctl"), {QStringLiteral("-m"), QStringLiteral("info")},
        [this](int code, const QString &out) {
            if (code != 0) {
                m_brightnessAvailable = false;
                emit changed();
                return;
            }
            const BrightnessState state = parseBrightnessctlInfo(out);
            if (!state.valid)
                return;
            m_brightness = state.maximum > 0 ? qRound(100.0 * state.current / state.maximum) : 0;
            emit changed();
        });
}

void SystemControl::probeBattery()
{
    if (!m_batteryAvailable)
        return;
    run(QStringLiteral("upower"), {QStringLiteral("-i"), QStringLiteral("/org/freedesktop/UPower/devices/DisplayDevice")},
        [this](int code, const QString &out) {
            BatteryState state = parseUpowerBattery(out);
            if (!state.valid) {
                // Desktop machines report no display battery; that is not an error.
                m_batteryAvailable = false;
                emit changed();
                return;
            }
            m_batteryAvailable = code == 0;
            m_batteryPercent = state.percent;
            m_batteryState = state.state;
            m_charging = state.state.compare(QLatin1String("charging"), Qt::CaseInsensitive) == 0
                || state.state.compare(QLatin1String("fully-charged"), Qt::CaseInsensitive) == 0;
            emit changed();
        });
}

void SystemControl::probePowerProfiles()
{
    if (!m_powerProfilesAvailable)
        return;
    run(QStringLiteral("powerprofilesctl"), {}, [this](int code, const QString &out) {
        if (code != 0) {
            m_powerProfilesAvailable = false;
            emit changed();
            return;
        }
        const ProfileState state = parsePowerProfiles(out);
        if (!state.valid)
            return;
        m_powerProfile = state.active;
        m_powerProfiles = state.available;
        emit changed();
    });
}

void SystemControl::probeRfkill()
{
    if (!m_airplaneAvailable)
        return;
    run(QStringLiteral("rfkill"), {QStringLiteral("list")}, [this](int code, const QString &out) {
        if (code != 0)
            return;
        bool wifiKnown = false;
        bool btKnown = false;
        const bool wifiBlocked = parseRfkillSoftBlocked(out, QStringLiteral("wifi"), &wifiKnown);
        const bool btBlocked = parseRfkillSoftBlocked(out, QStringLiteral("bluetooth"), &btKnown);
        if (!wifiKnown && !btKnown) {
            m_airplaneAvailable = false;
        } else {
            m_airplaneEnabled = wifiBlocked && (btBlocked || !btKnown);
        }
        emit changed();
    });
}

bool SystemControl::setWifiEnabled(bool enabled)
{
    if (!m_wifiAvailable)
        return false;
    m_wifiEnabled = enabled;
    emit changed();
    run(QStringLiteral("nmcli"), {QStringLiteral("radio"), QStringLiteral("wifi"),
                                  enabled ? QStringLiteral("on") : QStringLiteral("off")},
        [this](int code, const QString &out) {
            if (code != 0) {
                setError(tr("Kablosuz ağ değiştirilemedi: %1").arg(out.trimmed()));
                emit actionFailed(QStringLiteral("wifi"), m_lastError);
            }
            refresh();
        });
    return true;
}

bool SystemControl::setBluetoothEnabled(bool enabled)
{
    if (!m_bluetoothAvailable)
        return false;
    m_bluetoothEnabled = enabled;
    emit changed();
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("power"), enabled ? QStringLiteral("on")
                                                                        : QStringLiteral("off")},
        [this, enabled](int code, const QString &out) {
            if (code != 0) {
                setError(tr("Bluetooth değiştirilemedi: %1").arg(out.trimmed()));
                emit actionFailed(QStringLiteral("bluetooth"), m_lastError);
                m_bluetoothEnabled = !enabled;
                emit changed();
            }
            refresh();
        });
    return true;
}

void SystemControl::setVolume(int percent)
{
    if (!m_audioAvailable)
        return;
    m_pendingVolume = qBound(0, percent, 100);
    m_volume = m_pendingVolume;
    m_muted = m_pendingVolume == 0 ? m_muted : false;
    m_audioDirty = true;
    emit changed();
    // Coalesce slider drags into one wpctl call.
    QTimer::singleShot(90, this, [this] { flushVolumeChange(); });
}

void SystemControl::flushVolumeChange()
{
    if (m_pendingVolume < 0)
        return;
    const int target = m_pendingVolume;
    m_pendingVolume = -1;
    run(QStringLiteral("wpctl"),
        {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
         QStringLiteral("%1%").arg(target)},
        [this](int, const QString &) {
            m_audioDirty = false;
            probeAudio();
        });
}

bool SystemControl::setMuted(bool muted)
{
    if (!m_audioAvailable)
        return false;
    m_muted = muted;
    emit changed();
    run(QStringLiteral("wpctl"),
        {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
         muted ? QStringLiteral("1") : QStringLiteral("0")},
        [this](int, const QString &) { probeAudio(); });
    return true;
}

void SystemControl::setBrightness(int percent)
{
    if (!m_brightnessAvailable)
        return;
    m_brightness = qBound(1, percent, 100);
    emit changed();
    run(QStringLiteral("brightnessctl"), {QStringLiteral("set"), QStringLiteral("%1%").arg(m_brightness)},
        [this](int code, const QString &out) {
            if (code != 0) {
                setError(tr("Parlaklık ayarlanamadı: %1").arg(out.trimmed()));
                emit actionFailed(QStringLiteral("brightness"), m_lastError);
            }
        });
}

bool SystemControl::setPowerProfile(const QString &profile)
{
    if (!m_powerProfilesAvailable || profile.isEmpty())
        return false;
    m_powerProfile = profile;
    emit changed();
    run(QStringLiteral("powerprofilesctl"), {QStringLiteral("set"), profile},
        [this](int code, const QString &out) {
            if (code != 0) {
                setError(tr("Güç profili değiştirilemedi: %1").arg(out.trimmed()));
                emit actionFailed(QStringLiteral("power"), m_lastError);
            }
            refresh();
        });
    return true;
}

bool SystemControl::setNightLight(bool enabled)
{
    if (!m_nightLightAvailable)
        return false;

    if (enabled) {
        if (d->nightLight)
            return true;
        d->nightLight = new QProcess(this);
        d->nightLight->setProgram(QStringLiteral("gammastep"));
        d->nightLight->setArguments({QStringLiteral("-O"), QStringLiteral("3500")});
        QObject::connect(d->nightLight, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            setError(tr("Gece ışığı başlatılamadı"));
            emit actionFailed(QStringLiteral("nightlight"), m_lastError);
            m_nightLightEnabled = false;
            emit changed();
        });
        d->nightLight->start();
        m_nightLightEnabled = true;
    } else {
        if (d->nightLight) {
            d->nightLight->disconnect(this);
            d->nightLight->terminate();
            d->nightLight->waitForFinished(400);
            d->nightLight->deleteLater();
            d->nightLight = nullptr;
        }
        run(QStringLiteral("gammastep"), {QStringLiteral("-x")}, [](int, const QString &) {});
        m_nightLightEnabled = false;
    }
    emit changed();
    return true;
}

bool SystemControl::setAirplaneMode(bool enabled)
{
    if (!m_airplaneAvailable)
        return false;
    m_airplaneEnabled = enabled;
    emit changed();
    run(QStringLiteral("rfkill"),
        {enabled ? QStringLiteral("block") : QStringLiteral("unblock"), QStringLiteral("all")},
        [this](int code, const QString &out) {
            if (code != 0) {
                setError(tr("Uçak modu değiştirilemedi: %1").arg(out.trimmed()));
                emit actionFailed(QStringLiteral("airplane"), m_lastError);
                m_airplaneEnabled = !m_airplaneEnabled;
                emit changed();
            }
            refresh();
        });
    return true;
}

} // namespace TagOS::Control
