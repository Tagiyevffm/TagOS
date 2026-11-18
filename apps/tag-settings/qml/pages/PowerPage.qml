import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.Control 1.0
import TagOS.System 1.0

// Güç: profil, pil ve oturum işlemleri. Yıkıcı işlemler iki adımlı onay ister.
Item {
    id: page

    property var control: SystemControl
    property string pendingAction: ""

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    function request(action) {
        if (page.pendingAction === action) {
            page.pendingAction = "";
            if (action === "reboot")
                PowerManager.reboot();
            else if (action === "poweroff")
                PowerManager.powerOff();
            else if (action === "logout")
                PowerManager.logOut();
        } else {
            page.pendingAction = action;
            confirmTimer.restart();
        }
    }

    Timer {
        id: confirmTimer
        interval: 5000
        onTriggered: page.pendingAction = ""
    }

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Güç")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Güç profilleri power-profiles-daemon, oturum işlemleri logind üzerinden yürür.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }

        Item { width: 1; height: Spacing.xs }

        SettingRow {
            title: qsTr("Güç profili")
            description: page.control.powerProfilesAvailable
                         ? qsTr("Etkin: %1").arg(page.control.powerProfile)
                         : qsTr("powerprofilesctl bulunamadı")
            icon: "power-profile"

            TagSegmented {
                enabled: page.control.powerProfilesAvailable
                options: [
                    { label: qsTr("Tasarruf"), value: "power-saver" },
                    { label: qsTr("Dengeli"), value: "balanced" },
                    { label: qsTr("Performans"), value: "performance" }
                ]
                value: page.control.powerProfile
                onSelected: value => page.control.setPowerProfile(value)
            }
        }

        SettingRow {
            title: qsTr("Pil")
            description: page.control.batteryAvailable
                         ? qsTr("%1% · %2").arg(page.control.batteryPercent).arg(page.control.batteryState)
                         : qsTr("Pil bulunamadı (masaüstü sistem)")
            icon: page.control.charging ? "battery-charging" : "battery"

            TagRingGauge {
                width: 56
                height: 56
                thickness: 5
                valueFontSize: Typography.small
                value: page.control.batteryAvailable ? page.control.batteryPercent : -1
                progressColor: page.control.batteryPercent < 20 ? Theme.danger : Theme.success
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Item { width: 1; height: Spacing.xs }

        Text {
            text: qsTr("Oturum")
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.weight: Typography.medium
        }

        Row {
            spacing: Spacing.sm

            TagButton {
                text: page.pendingAction === "logout" ? qsTr("Onayla: oturumu kapat") : qsTr("Oturumu kapat")
                icon: "user"
                variant: page.pendingAction === "logout" ? TagButton.Danger : TagButton.Tonal
                onClicked: page.request("logout")
            }

            TagButton {
                text: page.pendingAction === "reboot" ? qsTr("Onayla: yeniden başlat") : qsTr("Yeniden başlat")
                icon: "refresh"
                variant: page.pendingAction === "reboot" ? TagButton.Danger : TagButton.Tonal
                onClicked: page.request("reboot")
            }

            TagButton {
                text: page.pendingAction === "poweroff" ? qsTr("Onayla: kapat") : qsTr("Kapat")
                icon: "power"
                variant: page.pendingAction === "poweroff" ? TagButton.Danger : TagButton.Ghost
                onClicked: page.request("poweroff")
            }
        }

        Text {
            width: parent.width
            visible: PowerManager.dryRun
            text: qsTr("TAGOS_DRY_RUN etkin: güç işlemleri gerçekten uygulanmaz, yalnızca kaydedilir.")
            color: Theme.warning
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            visible: PowerManager.lastError.length > 0
            text: PowerManager.lastError
            color: Theme.danger
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }
    }
}
