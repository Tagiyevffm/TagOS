pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0
import TagOS.System 1.0

// TagCenter: quick settings. Every tile reflects real state from SystemControl
// and is disabled when the underlying service is missing.
Item {
    id: root

    property var control: null
    property var notifications: null
    property var apps: null
    property int notificationLimit: 4

    signal requestSettings()
    signal requestDismiss()
    signal requestPowerMenu(point menuItem)

    implicitWidth: 384
    implicitHeight: column.implicitHeight + Spacing.lg * 2
    width: implicitWidth
    height: implicitHeight

    function launchApp(id) {
        if (!root.apps || !id)
            return false;
        return root.apps.launch(id);
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.card
        radius: Radius.medium
        border.width: 1
        border.color: Theme.borderStrong

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Spacing.lg
            spacing: Spacing.md

            // --- header ---------------------------------------------------
            Row {
                width: parent.width
                spacing: Spacing.sm

                Text {
                    text: qsTr("Denetim Merkezi")
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.subtitle
                    font.weight: Typography.semiBold
                }

                Item { width: parent.width - 150; height: 1 }

                TagIconButton {
                    icon: "close"
                    size: 26
                    iconSize: 14
                    tooltip: qsTr("Kapat")
                    onClicked: root.requestDismiss()
                }
            }

            // --- toggles --------------------------------------------------
            Grid {
                width: parent.width
                columns: 2
                columnSpacing: Spacing.sm
                rowSpacing: Spacing.sm

                TagChip {
                    icon: "wifi"
                    label: qsTr("Wi-Fi")
                    detail: root.control && root.control.wifiConnected ? root.control.wifiSsid : ""
                    active: root.control ? root.control.wifiEnabled : false
                    available: root.control ? root.control.wifiAvailable : false
                    onToggled: {
                        if (root.control)
                            root.control.setWifiEnabled(!root.control.wifiEnabled);
                    }
                }

                TagChip {
                    icon: "bluetooth"
                    label: qsTr("Bluetooth")
                    active: root.control ? root.control.bluetoothEnabled : false
                    available: root.control ? root.control.bluetoothAvailable : false
                    onToggled: {
                        if (root.control)
                            root.control.setBluetoothEnabled(!root.control.bluetoothEnabled);
                    }
                }

                TagChip {
                    icon: "airplane"
                    label: qsTr("Uçak modu")
                    active: root.control ? root.control.airplaneEnabled : false
                    available: root.control ? root.control.airplaneAvailable : false
                    onToggled: {
                        if (root.control)
                            root.control.setAirplaneMode(!root.control.airplaneEnabled);
                    }
                }

                TagChip {
                    icon: root.control && root.control.nightLightEnabled ? "moon" : "sun"
                    label: qsTr("Gece ışığı")
                    active: root.control ? root.control.nightLightEnabled : false
                    available: root.control ? root.control.nightLightAvailable : false
                    onToggled: {
                        if (root.control)
                            root.control.setNightLight(!root.control.nightLightEnabled);
                    }
                }

                TagChip {
                    icon: Theme.dark ? "moon" : "sun"
                    label: qsTr("Koyu tema")
                    active: Theme.dark
                    available: true
                    onToggled: SettingsStore.setThemeMode(Theme.dark ? "light" : "dark")
                }

                TagChip {
                    icon: "power-profile"
                    label: qsTr("Güç profili")
                    detail: root.control ? root.control.powerProfile : ""
                    active: root.control ? root.control.powerProfile === "performance" : false
                    available: root.control ? root.control.powerProfilesAvailable : false
                    onToggled: powerMenu.popup(undefined, 0, 0)
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border }

            // --- sliders --------------------------------------------------
            Row {
                width: parent.width
                spacing: Spacing.md

                TagIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: root.control && root.control.muted ? "volume-off" : "volume"
                    size: 16
                    opacity: root.control && root.control.audioAvailable ? 0.85 : 0.35
                }

                TagSlider {
                    width: parent.width - 76
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: root.control ? root.control.audioAvailable : false
                    value: root.control ? root.control.volume / 100 : 0
                    onMoved: value => {
                        if (root.control)
                            root.control.setVolume(Math.round(value * 100));
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34
                    horizontalAlignment: Text.AlignRight
                    text: root.control && root.control.audioAvailable
                          ? root.control.volume + "%" : "--"
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }

            Row {
                width: parent.width
                spacing: Spacing.md

                TagIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: root.control && root.control.nightLightEnabled ? "moon" : "sun-dim"
                    size: 16
                    opacity: root.control && root.control.brightnessAvailable ? 0.85 : 0.35
                }

                TagSlider {
                    width: parent.width - 76
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: root.control ? root.control.brightnessAvailable : false
                    value: root.control ? root.control.brightness / 100 : 0
                    onMoved: value => {
                        if (root.control)
                            root.control.setBrightness(Math.round(value * 100));
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34
                    horizontalAlignment: Text.AlignRight
                    text: root.control && root.control.brightnessAvailable
                          ? root.control.brightness + "%" : "--"
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }

            // --- battery --------------------------------------------------
            Row {
                width: parent.width
                spacing: Spacing.md
                visible: root.control ? root.control.batteryAvailable : false

                TagIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: root.control && root.control.charging ? "battery-charging" : "battery"
                    size: 16
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.control ? qsTr("Pil %1%").arg(root.control.batteryPercent) : ""
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.control ? root.control.batteryState : ""
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border }

            // --- notifications -------------------------------------------
            Row {
                width: parent.width
                spacing: Spacing.sm

                Text {
                    text: qsTr("Bildirimler")
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.medium
                }

                Item { width: parent.width - 150; height: 1 }

                Text {
                    text: root.notifications && root.notifications.count > 0
                          ? qsTr("Tümünü temizle") : ""
                    color: Theme.accent
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    visible: text.length > 0

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.notifications) root.notifications.clear()
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Spacing.xs

                Item {
                    width: parent.width
                    height: 54
                    visible: root.notifications ? root.notifications.count === 0 : true

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Yeni bildirim yok")
                        color: Theme.textMuted
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }
                }

                Repeater {
                    model: root.notifications ? root.notifications.model : null
                    delegate: Item {
                        id: item
                        required property int index
                        required property string title
                        required property string body
                        required property string stamp

                        width: parent ? parent.width : 0
                        height: index < root.notificationLimit ? 52 : 0
                        visible: index < root.notificationLimit

                        Row {
                            anchors.fill: parent
                            spacing: Spacing.sm

                            TagIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "bell"
                                size: 15
                                opacity: 0.7
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 70
                                spacing: 1

                                Text {
                                    width: parent.width
                                    text: item.title
                                    color: Theme.text
                                    font.family: Typography.family
                                    font.pixelSize: Typography.caption
                                    font.weight: Typography.medium
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: item.body
                                    color: Theme.textMuted
                                    font.family: Typography.family
                                    font.pixelSize: Typography.caption - 1
                                    elide: Text.ElideRight
                                }
                            }

                            TagIconButton {
                                anchors.verticalCenter: parent.verticalCenter
                                icon: "close"
                                size: 22
                                iconSize: 12
                                onClicked: if (root.notifications) root.notifications.dismiss(item.index)
                            }
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border }

            // --- footer ---------------------------------------------------
            Row {
                width: parent.width
                spacing: Spacing.sm

                TagButton {
                    text: qsTr("Ayarlar")
                    icon: "settings"
                    variant: TagButton.Tonal
                    onClicked: {
                        root.requestDismiss();
                        root.launchApp("tag-settings.desktop");
                    }
                }

                TagButton {
                    text: qsTr("Güç")
                    icon: "power"
                    variant: TagButton.Ghost
                    onClicked: powerMenu.popup(undefined, 0, 0)
                }

                Item { width: parent.width - 220; height: 1 }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.control ? root.control.transportSummary : ""
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption - 1
                    elide: Text.ElideRight
                    width: 130
                }
            }
        }
    }

    TagMenu {
        id: powerMenu
        items: [
            { label: qsTr("Yeniden başlat"), icon: "refresh" },
            { label: qsTr("Kapat"), icon: "power", danger: true },
            { label: qsTr("Oturumu kapat"), icon: "user" },
            { separator: true },
            { label: qsTr("Güç profili: Dengeli"), icon: "power-profile" },
            { label: qsTr("Güç profili: Performans"), icon: "power-profile" },
            { label: qsTr("Güç profili: Tasarruf"), icon: "power-profile" }
        ]
        onTriggered: index => {
            if (index === 0) {
                PowerManager.reboot();
            } else if (index === 1) {
                PowerManager.powerOff();
            } else if (index === 2) {
                PowerManager.logOut();
            } else if (index === 4 && root.control) {
                root.control.setPowerProfile("balanced");
            } else if (index === 5 && root.control) {
                root.control.setPowerProfile("performance");
            } else if (index === 6 && root.control) {
                root.control.setPowerProfile("power-saver");
            }
        }
    }
}
