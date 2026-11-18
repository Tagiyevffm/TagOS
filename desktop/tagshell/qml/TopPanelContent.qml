import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0

// Top panel contents: brand + quick launch, global search, status tray, clock.
Item {
    id: root

    property var flow: null
    property var control: null
    property var notifications: null
    property var apps: null

    readonly property int panelHeight: Math.max(38, SettingsStore.panelHeight)

    signal requestLauncher()
    signal requestCenter()
    signal requestPower()

    implicitHeight: panelHeight

    function launchOrWarn(id, label) {
        if (!root.apps)
            return;
        if (!root.apps.launch(id) && root.notifications)
            root.notifications.notify(label, qsTr("Uygulama başlatılamadı"), "warning");
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surfacePanel

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }
    }

    // --- left cluster -----------------------------------------------------
    Row {
        anchors.left: parent.left
        anchors.leftMargin: Spacing.md
        anchors.verticalCenter: parent.verticalCenter
        spacing: Spacing.xs

        Item {
            width: brandRow.implicitWidth + Spacing.md
            height: root.panelHeight

            Row {
                id: brandRow
                anchors.centerIn: parent
                spacing: Spacing.sm

                TagIcon { name: "tagos-logo"; size: 18; anchors.verticalCenter: parent.verticalCenter }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "TagOS"
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.semiBold
                    font.letterSpacing: 0.6
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.requestLauncher()
            }
        }

        Rectangle {
            width: 1
            height: 18
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.border
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "grid"
            tooltip: qsTr("Uygulamalar (Ctrl+Space)")
            onClicked: root.requestLauncher()
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "folder"
            tooltip: qsTr("TagFiles")
            onClicked: root.launchOrWarn("tag-files.desktop", qsTr("TagFiles"))
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "monitor"
            tooltip: qsTr("TagMonitor")
            onClicked: root.launchOrWarn("tag-monitor.desktop", qsTr("TagMonitor"))
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "settings"
            tooltip: qsTr("TagSettings")
            onClicked: root.launchOrWarn("tag-settings.desktop", qsTr("TagSettings"))
        }
    }

    // --- centre: global search -------------------------------------------
    Rectangle {
        id: searchPill
        anchors.centerIn: parent
        width: Math.min(360, root.width * 0.32)
        height: 28
        radius: Radius.pill
        color: Theme.surfaceAlt
        border.width: 1
        border.color: searchHover.containsMouse ? Theme.accent : Theme.border

        Row {
            anchors.centerIn: parent
            spacing: Spacing.sm

            TagIcon { name: "search"; size: 14; opacity: 0.7; anchors.verticalCenter: parent.verticalCenter }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Her şeyi ara…")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: Spacing.md
            anchors.verticalCenter: parent.verticalCenter
            text: "Ctrl+Space"
            color: Theme.textMuted
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption - 1
            opacity: 0.75
        }

        MouseArea {
            id: searchHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.requestLauncher()
        }
    }

    // --- right cluster ----------------------------------------------------
    Row {
        anchors.right: parent.right
        anchors.rightMargin: Spacing.md
        anchors.verticalCenter: parent.verticalCenter
        spacing: Spacing.xxs

        Item {
            width: 32
            height: 32

            TagIconButton {
                anchors.centerIn: parent
                icon: "bell"
                tooltip: qsTr("Bildirimler")
                onClicked: root.requestCenter()
            }

            Rectangle {
                visible: root.notifications ? root.notifications.unseen > 0 : false
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: 3
                anchors.topMargin: 3
                width: 14
                height: 14
                radius: 7
                color: Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: root.notifications ? String(Math.min(9, root.notifications.unseen)) : ""
                    color: Theme.accentInk
                    font.family: Typography.family
                    font.pixelSize: 9
                    font.weight: Typography.bold
                }
            }
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: root.control && root.control.wifiConnected ? "wifi" : "wifi-off"
            tooltip: root.control && root.control.wifiConnected
                     ? qsTr("Bağlı: %1").arg(root.control.wifiSsid)
                     : qsTr("Kablosuz ağ")
            onClicked: root.requestCenter()
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: root.control && root.control.muted ? "volume-off" : "volume"
            tooltip: root.control && root.control.audioAvailable
                     ? qsTr("Ses %1%").arg(root.control.volume)
                     : qsTr("Ses aygıtı bulunamadı")
            onClicked: root.requestCenter()
        }

        Row {
            visible: root.control ? root.control.batteryAvailable : false
            spacing: Spacing.xs
            anchors.verticalCenter: parent.verticalCenter

            TagIcon {
                anchors.verticalCenter: parent.verticalCenter
                name: root.control && root.control.charging ? "battery-charging" : "battery"
                size: 15
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.control ? root.control.batteryPercent + "%" : ""
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        Rectangle {
            width: 1
            height: 18
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.border
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            rightPadding: Spacing.xs

            Text {
                id: clockText
                anchors.right: parent.right
                text: Qt.formatDateTime(new Date(), "HH:mm")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: Typography.medium
            }

            Text {
                id: dateText
                anchors.right: parent.right
                text: Qt.formatDateTime(new Date(), "dd MMM")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
            }
        }

        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: {
                const now = new Date();
                clockText.text = Qt.formatDateTime(now, SettingsStore.showSeconds ? "HH:mm:ss" : "HH:mm");
                dateText.text = Qt.formatDateTime(now, "dd MMM");
            }
        }

        TagIconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon: "power"
            tooltip: qsTr("Güç")
            danger: true
            onClicked: root.requestPower()
        }
    }
}
