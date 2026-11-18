#include "tag-control/ControlParsers.h"

#include <QTest>

using namespace TagOS::Control;

class ControlParsersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesActiveWifi();
    void parsesEmptyWifiList();
    void parsesRadioState();
    void parsesWpctlVolume();
    void parsesWpctlMuted();
    void parsesBluetoothShow_data();
    void parsesBluetoothShow();
    void parsesBattery();
    void ignoresDesktopWithoutBattery();
    void parsesBrightness();
    void parsesPowerProfiles();
    void parsesRfkill();
    void detectsWifiDevice();
};

void ControlParsersTest::parsesActiveWifi()
{
    const QString text = QStringLiteral(
        ":EvWifi:74:WPA2\n"
        "*:TagOS-Lab:91:WPA2\n"
        ":Komşu:33:WPA1 WPA2\n");
    const WifiState state = parseNmcliWifiList(text);
    QVERIFY(state.valid);
    QVERIFY(state.connected);
    QCOMPARE(state.ssid, QStringLiteral("TagOS-Lab"));
    QCOMPARE(state.strength, 91);
    QCOMPARE(state.security, QStringLiteral("WPA2"));
}

void ControlParsersTest::parsesEmptyWifiList()
{
    const WifiState state = parseNmcliWifiList(QString());
    QVERIFY(!state.connected);
    QVERIFY(state.ssid.isEmpty());
    QCOMPARE(state.strength, 0);
}

void ControlParsersTest::parsesRadioState()
{
    bool ok = false;
    QVERIFY(parseNmcliRadio(QStringLiteral("enabled\n"), &ok));
    QVERIFY(ok);
    QVERIFY(!parseNmcliRadio(QStringLiteral("disabled\n"), &ok));
    QVERIFY(ok);
    parseNmcliRadio(QStringLiteral("garbage"), &ok);
    QVERIFY(!ok);
}

void ControlParsersTest::parsesWpctlVolume()
{
    const AudioState state = parseWpctlVolume(QStringLiteral("Volume: 0.45\n"));
    QVERIFY(state.valid);
    QCOMPARE(state.volume, 45);
    QVERIFY(!state.muted);

    const AudioState loud = parseWpctlVolume(QStringLiteral("Volume: 1.00\n"));
    QCOMPARE(loud.volume, 100);

    QVERIFY(!parseWpctlVolume(QStringLiteral("No such object")).valid);
}

void ControlParsersTest::parsesWpctlMuted()
{
    const AudioState state = parseWpctlVolume(QStringLiteral("Volume: 0.30 [MUTED]\n"));
    QVERIFY(state.valid);
    QCOMPARE(state.volume, 30);
    QVERIFY(state.muted);
}

void ControlParsersTest::parsesBluetoothShow_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("expected");
    QTest::addColumn<bool>("known");

    QTest::newRow("powered") << QStringLiteral("Controller 00:1A:7D:DA:71:13\n\tPowered: yes\n") << true << true;
    QTest::newRow("off") << QStringLiteral("\tPowered: no\n") << false << true;
    QTest::newRow("powerstate") << QStringLiteral("PowerState: on\n") << true << true;
    QTest::newRow("unknown") << QStringLiteral("No default controller available\n") << false << false;
}

void ControlParsersTest::parsesBluetoothShow()
{
    QFETCH(QString, text);
    QFETCH(bool, expected);
    QFETCH(bool, known);

    bool ok = false;
    const bool powered = parseBluetoothPowered(text, &ok);
    QCOMPARE(powered, expected);
    QCOMPARE(ok, known);
}

void ControlParsersTest::parsesBattery()
{
    const QString text = QStringLiteral(
        "Device: /org/freedesktop/UPower/devices/battery_BAT0\n"
        "  native-path:          BAT0\n"
        "  present:              yes\n"
        "  state:                discharging\n"
        "  percentage:           63%\n");
    const BatteryState state = parseUpowerBattery(text);
    QVERIFY(state.valid);
    QVERIFY(state.present);
    QCOMPARE(state.percent, 63);
    QCOMPARE(state.state, QStringLiteral("discharging"));
}

void ControlParsersTest::ignoresDesktopWithoutBattery()
{
    const QString text = QStringLiteral("  present:              no\n");
    const BatteryState state = parseUpowerBattery(text);
    QVERIFY(!state.valid);
    QVERIFY(!state.present);
    QCOMPARE(state.percent, -1);
}

void ControlParsersTest::parsesBrightness()
{
    const BrightnessState state = parseBrightnessctlInfo(
        QStringLiteral("intel_backlight,backlight,4687,50%,9375\n"));
    QVERIFY(state.valid);
    QCOMPARE(state.current, 4687);
    QCOMPARE(state.maximum, 9375);

    QVERIFY(!parseBrightnessctlInfo(QStringLiteral("nonsense")).valid);
}

void ControlParsersTest::parsesPowerProfiles()
{
    const QString text = QStringLiteral(
        "  performance:\n"
        "    Driver:     platform_profile\n"
        "    Degraded:   no\n"
        "\n"
        "* balanced:\n"
        "    Driver:     platform_profile\n"
        "\n"
        "  power-saver:\n"
        "    Driver:     platform_profile\n");
    const ProfileState state = parsePowerProfiles(text);
    QVERIFY(state.valid);
    QCOMPARE(state.active, QStringLiteral("balanced"));
    QCOMPARE(state.available,
             (QStringList{QStringLiteral("performance"), QStringLiteral("balanced"),
                          QStringLiteral("power-saver")}));
}

void ControlParsersTest::parsesRfkill()
{
    const QString text = QStringLiteral(
        "0: phy0: Wireless LAN\n"
        "\tSoft blocked: no\n"
        "\tHard blocked: no\n"
        "\n"
        "1: hci0: Bluetooth\n"
        "\tSoft blocked: yes\n"
        "\tHard blocked: no\n");

    bool ok = false;
    QVERIFY(!parseRfkillSoftBlocked(text, QStringLiteral("wifi"), &ok));
    QVERIFY(ok);
    QVERIFY(parseRfkillSoftBlocked(text, QStringLiteral("bluetooth"), &ok));
    QVERIFY(ok);
    parseRfkillSoftBlocked(text, QStringLiteral("wwan"), &ok);
    QVERIFY(!ok);
}

void ControlParsersTest::detectsWifiDevice()
{
    QVERIFY(parseNmcliHasWifiDevice(QStringLiteral("wlan0:wifi:connected\neth0:ethernet:unavailable\n")));
    QVERIFY(!parseNmcliHasWifiDevice(QStringLiteral("eth0:ethernet:connected\n")));
}

QTEST_GUILESS_MAIN(ControlParsersTest)
#include "tst_control_parsers.moc"
