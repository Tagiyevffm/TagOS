import QtQuick
import QtQuick.Window
import TagOS.Design 1.0
import TagOS.Flow 1.0

// TagMonitor — CPU / RAM / disk with live history, built only from TagOS.Design
// components and the TagFlow client (identical data path as the shell).
Window {
    id: window

    width: 760
    height: 560
    visible: true
    color: Theme.bg
    title: qsTr("TagMonitor")

    FlowClient {
        id: metrics
        pollIntervalMs: 1000
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.bgElevated }
            GradientStop { position: 1.0; color: Theme.bg }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: Spacing.xl
        spacing: Spacing.lg

        Row {
            width: parent.width
            spacing: Spacing.md

            Image {
                source: Theme.icon("tagos-logo")
                sourceSize.width: 26
                sourceSize.height: 26
                width: 24
                height: 24
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    text: qsTr("TagMonitor")
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.headline
                    font.weight: Typography.bold
                }
                Text {
                    text: metrics.distroName + " · " + metrics.kernelVersion + " · " + metrics.transportLabel
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }

        TagCard {
            width: parent.width
            height: 148
            padding: Spacing.lg

            Row {
                anchors.fill: parent
                spacing: Spacing.xl

                TagRingGauge {
                    width: 108; height: 108
                    value: metrics.cpuPercent
                    label: qsTr("CPU")
                    caption: metrics.cpuCount + qsTr(" iş parçacığı")
                    anchors.verticalCenter: parent.verticalCenter
                }

                TagRingGauge {
                    width: 108; height: 108
                    value: metrics.memoryUsedPercent
                    label: qsTr("RAM")
                    progressColor: Theme.accentHover
                    anchors.verticalCenter: parent.verticalCenter
                }

                TagRingGauge {
                    width: 108; height: 108
                    value: metrics.diskUsedPercent
                    label: qsTr("Disk")
                    progressColor: Theme.info
                    anchors.verticalCenter: parent.verticalCenter
                }

                Column {
                    width: parent.width - 380
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Spacing.sm

                    Text {
                        text: qsTr("CPU geçmişi")
                        color: Theme.textDim
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }

                    TagSparkline {
                        width: parent.width
                        height: 62
                        values: metrics.cpuHistory
                        lineColor: Theme.accent
                    }
                }
            }
        }

        TagCard {
            width: parent.width
            height: 120
            padding: Spacing.lg

            Column {
                anchors.fill: parent
                spacing: Spacing.xs

                Text {
                    text: qsTr("Bellek")
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.subtitle
                    font.weight: Typography.medium
                }

                TagSparkline {
                    width: parent.width
                    height: 46
                    values: metrics.memoryHistory
                    lineColor: Theme.accentHover
                }

                Text {
                    text: metrics.memoryText + "  ·  " + qsTr("takas %1").arg(Math.round(metrics.swapUsedPercent) + "%")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }

        MeterRow {
            width: parent.width
            label: qsTr("Çalışma süresi")
            value: metrics.uptimeText
        }

        MeterRow {
            width: parent.width
            label: qsTr("Yük ortalaması")
            value: metrics.loadText
        }

        MeterRow {
            width: parent.width
            label: qsTr("Sunucu")
            value: metrics.hostName + " (" + metrics.distroName + ")"
        }

        Item { width: 1; height: 1 }

        Text {
            width: parent.width
            text: qsTr("TagOS %1 · %2").arg(metrics.tagosVersion).arg(Theme.codename)
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption - 1
        }
    }
}
