pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0
import TagOS.System 1.0

// TagCenter — quick settings and power.
//
// Phase 0 deliberately ships the parts that are real: live metrics and session
// power actions. Network / Bluetooth / audio tiles are visible but marked as
// Phase 2 work, because TagOS integrates NetworkManager, BlueZ and PipeWire
// instead of faking switches that do nothing.
TagCard {
    id: root

    property var metrics: null
    property bool opened: false

    width: 372
    padding: Spacing.lg
    surfaceColor: Theme.bgElevated
    borderColor: Theme.borderStrong

    Column {
        anchors.fill: parent
        spacing: Spacing.md

        Row {
            width: parent.width
            spacing: Spacing.sm

            Image {
                source: Theme.icon("tagos-logo")
                sourceSize.width: 22
                sourceSize.height: 22
                width: 20
                height: 20
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("TagCenter")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.bold
            }

            Item { width: parent.width - 110; height: 1 }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.metrics ? root.metrics.hostName : ""
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                width: 110
                horizontalAlignment: Text.AlignRight
            }
        }

        Grid {
            width: parent.width
            columns: 4
            columnSpacing: Spacing.sm

            QuickTile { icon: "wifi"; label: qsTr("Wi-Fi"); phase: 2 }
            QuickTile { icon: "bluetooth"; label: "Bluetooth"; phase: 2 }
            QuickTile { icon: "volume"; label: qsTr("Ses"); phase: 2 }
            QuickTile { icon: "cpu"; label: qsTr("Performans"); phase: 2 }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Column {
            width: parent.width
            spacing: Spacing.xs

            Row {
                width: parent.width
                spacing: Spacing.sm

                TagIcon { name: "cpu"; size: 15; anchors.verticalCenter: parent.verticalCenter }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Sistem durumu")
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }

            Row {
                width: parent.width
                spacing: Spacing.md

                TagRingGauge {
                    width: 88; height: 88
                    value: root.metrics ? root.metrics.cpuPercent : -1
                    label: "CPU"
                    valueFontSize: Typography.subtitle
                    thickness: 5
                }

                TagRingGauge {
                    width: 88; height: 88
                    value: root.metrics ? root.metrics.memoryUsedPercent : -1
                    label: "RAM"
                    valueFontSize: Typography.subtitle
                    thickness: 5
                    progressColor: Theme.accentHover
                }

                TagRingGauge {
                    width: 88; height: 88
                    value: root.metrics ? root.metrics.diskUsedPercent : -1
                    label: "Disk"
                    valueFontSize: Typography.subtitle
                    thickness: 5
                    progressColor: Theme.info
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Row {
            width: parent.width
            spacing: Spacing.sm

            TagButton {
                text: qsTr("Yeniden Başlat")
                icon: "refresh"
                variant: TagButton.Tonal
                onClicked: PowerManager.reboot()
            }

            TagButton {
                text: qsTr("Kapat")
                icon: "power"
                variant: TagButton.Danger
                onClicked: PowerManager.powerOff()
            }
        }

        Text {
            width: parent.width
            visible: text.length > 0
            text: PowerManager.lastError
            color: Theme.danger
            wrapMode: Text.WordWrap
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Text {
            width: parent.width
            text: qsTr("Ağ, ses ve Bluetooth Faz 2'de NetworkManager, PipeWire ve BlueZ üzerinden bağlanacak.")
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            font.family: Typography.family
            font.pixelSize: Typography.caption - 1
        }
    }

    component QuickTile: Rectangle {
        id: tile

        property string icon: "wifi"
        property string label: ""
        property int phase: 2

        width: (root.width - Spacing.lg * 2 - Spacing.sm * 3) / 4
        height: 66
        radius: Radius.medium
        color: area.containsMouse ? Theme.surfaceHover : Theme.surface
        border.width: 1
        border.color: Theme.border
        opacity: 0.75

        Column {
            anchors.centerIn: parent
            spacing: Spacing.xxs

            TagIcon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: tile.icon
                size: 20
                dim: 0.9
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tile.label
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Faz %1").arg(tile.phase)
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption - 2
            }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.ForbiddenCursor
        }
    }
}
