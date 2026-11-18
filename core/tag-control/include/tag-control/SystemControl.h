#pragma once

#include <functional>

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "tag-common/Export.h"

namespace TagOS::Control {

/// Real system state for the TagCenter tiles.
///
/// Every capability is backed by a Linux service or CLI tool. Nothing is faked:
/// if NetworkManager / BlueZ / PipeWire / UPower / power-profiles-daemon /
/// brightnessctl / rfkill is missing, the matching property reports
/// `available == false` and the UI renders a disabled "desteklenmiyor" tile
/// instead of a switch that does nothing.
class TAGOS_EXPORT SystemControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool refreshing READ refreshing NOTIFY changed)

    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY changed)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY changed)
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY changed)
    Q_PROPERTY(QString wifiSsid READ wifiSsid NOTIFY changed)
    Q_PROPERTY(int wifiStrength READ wifiStrength NOTIFY changed)
    Q_PROPERTY(QString wifiSecurity READ wifiSecurity NOTIFY changed)
    Q_PROPERTY(QStringList wifiNetworks READ wifiNetworks NOTIFY changed)

    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable NOTIFY changed)
    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled NOTIFY changed)

    Q_PROPERTY(bool audioAvailable READ audioAvailable NOTIFY changed)
    Q_PROPERTY(int volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)

    Q_PROPERTY(bool brightnessAvailable READ brightnessAvailable NOTIFY changed)
    Q_PROPERTY(int brightness READ brightness NOTIFY changed)

    Q_PROPERTY(bool batteryAvailable READ batteryAvailable NOTIFY changed)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY changed)
    Q_PROPERTY(QString batteryState READ batteryState NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)

    Q_PROPERTY(bool powerProfilesAvailable READ powerProfilesAvailable NOTIFY changed)
    Q_PROPERTY(QString powerProfile READ powerProfile NOTIFY changed)
    Q_PROPERTY(QStringList powerProfiles READ powerProfiles NOTIFY changed)

    Q_PROPERTY(bool nightLightAvailable READ nightLightAvailable NOTIFY changed)
    Q_PROPERTY(bool nightLightEnabled READ nightLightEnabled NOTIFY changed)

    Q_PROPERTY(bool airplaneAvailable READ airplaneAvailable NOTIFY changed)
    Q_PROPERTY(bool airplaneEnabled READ airplaneEnabled NOTIFY changed)

    /// id/label/available/detail rows for TagSettings → System.
    Q_PROPERTY(QVariantList capabilities READ capabilities NOTIFY changed)
    Q_PROPERTY(QString transportSummary READ transportSummary NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit SystemControl(QObject *parent = nullptr);
    ~SystemControl() override;

    bool refreshing() const { return m_refreshing; }
    bool wifiAvailable() const { return m_wifiAvailable; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    bool wifiConnected() const { return m_wifiConnected; }
    QString wifiSsid() const { return m_wifiSsid; }
    int wifiStrength() const { return m_wifiStrength; }
    QString wifiSecurity() const { return m_wifiSecurity; }
    QStringList wifiNetworks() const { return m_wifiNetworks; }
    bool bluetoothAvailable() const { return m_bluetoothAvailable; }
    bool bluetoothEnabled() const { return m_bluetoothEnabled; }
    bool audioAvailable() const { return m_audioAvailable; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    bool brightnessAvailable() const { return m_brightnessAvailable; }
    int brightness() const { return m_brightness; }
    bool batteryAvailable() const { return m_batteryAvailable; }
    int batteryPercent() const { return m_batteryPercent; }
    QString batteryState() const { return m_batteryState; }
    bool charging() const { return m_charging; }
    bool powerProfilesAvailable() const { return m_powerProfilesAvailable; }
    QString powerProfile() const { return m_powerProfile; }
    QStringList powerProfiles() const { return m_powerProfiles; }
    bool nightLightAvailable() const { return m_nightLightAvailable; }
    bool nightLightEnabled() const { return m_nightLightEnabled; }
    bool airplaneAvailable() const { return m_airplaneAvailable; }
    bool airplaneEnabled() const { return m_airplaneEnabled; }
    QVariantList capabilities() const;
    QString transportSummary() const;
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setWifiEnabled(bool enabled);
    Q_INVOKABLE bool setBluetoothEnabled(bool enabled);
    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE bool setMuted(bool muted);
    Q_INVOKABLE void setBrightness(int percent);
    Q_INVOKABLE bool setPowerProfile(const QString &profile);
    Q_INVOKABLE bool setNightLight(bool enabled);
    Q_INVOKABLE bool setAirplaneMode(bool enabled);

signals:
    void changed();
    void lastErrorChanged();
    void actionFailed(const QString &action, const QString &reason);

private:
    void run(const QString &program, const QStringList &args,
             std::function<void(int, const QString &)> done);
    void probeWifi();
    void probeBluetooth();
    void probeAudio();
    void probeBrightness();
    void probeBattery();
    void probePowerProfiles();
    void probeRfkill();
    void setError(const QString &message);
    void flushVolumeChange();

    bool m_refreshing = false;
    bool m_wifiAvailable = false;
    bool m_wifiEnabled = false;
    bool m_wifiConnected = false;
    QString m_wifiSsid;
    int m_wifiStrength = 0;
    QString m_wifiSecurity;
    QStringList m_wifiNetworks;

    bool m_bluetoothAvailable = false;
    bool m_bluetoothEnabled = false;

    bool m_audioAvailable = false;
    int m_volume = 0;
    bool m_muted = false;
    bool m_audioDirty = false;
    int m_pendingVolume = -1;

    bool m_brightnessAvailable = false;
    int m_brightness = 0;

    bool m_batteryAvailable = false;
    int m_batteryPercent = -1;
    QString m_batteryState;
    bool m_charging = false;

    bool m_powerProfilesAvailable = false;
    QString m_powerProfile;
    QStringList m_powerProfiles;

    bool m_nightLightAvailable = false;
    bool m_nightLightEnabled = false;

    bool m_airplaneAvailable = false;
    bool m_airplaneEnabled = false;

    QString m_lastError;
    QStringList m_detectedTools;
    class Private;
    Private *d = nullptr;
};

} // namespace TagOS::Control
