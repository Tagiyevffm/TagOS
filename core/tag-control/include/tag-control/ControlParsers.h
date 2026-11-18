#pragma once

#include <QString>
#include <QStringList>

#include "tag-common/Export.h"

namespace TagOS::Control {

// Pure parsers for the CLI tools TagOS drives. Keeping them free of QProcess
// makes every format regression testable on any host.

struct TAGOS_EXPORT WifiState
{
    bool enabled = false;
    bool connected = false;
    QString ssid;
    QString connection;   ///< active connection name
    int strength = 0;     ///< 0..100
    QString security;
    bool valid = false;   ///< parser recognised the output
};

/// `nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list`
TAGOS_EXPORT WifiState parseNmcliWifiList(const QString &text);
/// `nmcli -t -f WIFI radio` -> "enabled"/"disabled"
TAGOS_EXPORT bool parseNmcliRadio(const QString &text, bool *ok = nullptr);

struct TAGOS_EXPORT AudioState
{
    bool valid = false;
    int volume = 0;     ///< 0..100
    bool muted = false;
    QString sinkName;
};

/// `wpctl get-volume @DEFAULT_AUDIO_SINK@` -> "Volume: 0.45 [MUTED]"
TAGOS_EXPORT AudioState parseWpctlVolume(const QString &text);

/// `bluetoothctl show` -> "Powered: yes"; also accepts "PowerState: on".
TAGOS_EXPORT bool parseBluetoothPowered(const QString &text, bool *ok = nullptr);

struct TAGOS_EXPORT BatteryState
{
    bool present = false;
    int percent = -1;
    QString state;    ///< Charging, Discharging, Full, Unknown
    bool valid = false;
};

/// `upower -i /org/freedesktop/UPower/devices/battery_BAT0`
TAGOS_EXPORT BatteryState parseUpowerBattery(const QString &text);

struct TAGOS_EXPORT BrightnessState
{
    bool valid = false;
    int current = 0;
    int maximum = 0;
};

/// `brightnessctl -m info` -> "acpi_backlight,122,10%,9375" (or `-m` shaped rows)
TAGOS_EXPORT BrightnessState parseBrightnessctlInfo(const QString &text);

struct TAGOS_EXPORT ProfileState
{
    bool valid = false;
    QString active;
    QStringList available;
};

/// `powerprofilesctl` full listing
TAGOS_EXPORT ProfileState parsePowerProfiles(const QString &text);

/// `rfkill list` -> whether the wifi/Bluetooth radio of `type` is soft-blocked.
TAGOS_EXPORT bool parseRfkillSoftBlocked(const QString &text, const QString &type, bool *ok = nullptr);

/// `nmcli -t -f DEVICE,TYPE,STATE device status`
TAGOS_EXPORT bool parseNmcliHasWifiDevice(const QString &text);

} // namespace TagOS::Control
