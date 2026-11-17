import QtQuick
import TagOS.Design 1.0

// Live system monitor card on the right edge of the board.
TagCard {
    id: root

    property var metrics: null

    width: 320
    padding: Spacing.lg

    Column {
        anchors.fill: parent
        spacing: Spacing.md

        Row {
            width: parent.width
            spacing: Spacing.sm

            TagIcon { name: "monitor"; size: 18; anchors.verticalCenter: parent.verticalCenter }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Sistem Monitörü")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.medium
            }

            Item { width: parent.width - 200; height: 1 }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.metrics ? root.metrics.transportLabel : ""
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
            }
        }

        Row {
            width: parent.width
            spacing: Spacing.sm

            TagRingGauge {
                width: 88
                height: 88
                value: root.metrics ? root.metrics.cpuPercent : -1
                label: "CPU"
                progressColor: Theme.accent
            }

            TagRingGauge {
                width: 88
                height: 88
                value: root.metrics ? root.metrics.memoryUsedPercent : -1
                label: "RAM"
                progressColor: Theme.accentHover
            }

            TagRingGauge {
                width: 88
                height: 88
                value: root.metrics ? root.metrics.diskUsedPercent : -1
                label: "Disk"
                progressColor: Theme.info
            }
        }

        TagSparkline {
            width: parent.width
            height: 58
            values: root.metrics ? root.metrics.cpuHistory : []
            lineColor: Theme.accent
            maxValue: 100
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Grid {
            width: parent.width
            columns: 2
            columnSpacing: Spacing.lg
            rowSpacing: Spacing.xs

            Text {
                text: qsTr("Çalışma süresi")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: root.metrics ? root.metrics.uptimeText : "--"
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: qsTr("Bellek")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: root.metrics ? root.metrics.memoryText : "--"
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: qsTr("Çekirdek")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: root.metrics ? root.metrics.kernelVersion : "--"
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                width: 200
            }

            Text {
                text: qsTr("Yük")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                text: root.metrics ? root.metrics.loadText : "--"
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }
    }
}
