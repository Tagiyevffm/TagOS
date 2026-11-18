pragma ComponentBehavior: Bound

import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.Control 1.0
import TagOS.Config 1.0
import TagOS.Flow 1.0
import TagOS.System 1.0

// Sistem: sürümler, çekirdek, donanım ve servis kapsamı (gerçek durum).
Item {
    id: page

    property var control: SystemControl

    FlowClient { id: flow }

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Sistem")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Bu sayfa tahmin yürütmez: her satır çalışan sistemden okunur.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Item { width: 1; height: Spacing.xs }

        SettingRow {
            title: qsTr("İşletim sistemi")
            icon: "tagos-logo"

            Text {
                text: "TagOS " + SessionInfo.tagosVersion
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Temel dağıtım")
            icon: "package"

            Text {
                text: SessionInfo.prettyOsName
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                width: 280
            }
        }

        SettingRow {
            title: qsTr("Çekirdek")
            icon: "cpu"

            Text {
                text: flow.kernelVersion.length > 0 ? flow.kernelVersion : SessionInfo.kernelVersion
                color: Theme.textDim
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Mimari")
            icon: "layers"

            Text {
                text: SessionInfo.architecture
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("İşlemci")
            icon: "cpu"

            Text {
                text: SessionInfo.cpuModel
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                width: 300
            }
        }

        SettingRow {
            title: qsTr("Bellek")
            icon: "speed"

            Text {
                text: flow.valid ? flow.memoryText : SessionInfo.totalMemoryText
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Derleme türü")
            icon: "wrench"

            Text {
                text: SessionInfo.buildType
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Text {
            text: qsTr("Servis kapsamı")
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.weight: Typography.medium
        }

        Repeater {
            model: page.control.capabilities

            delegate: Row {
                required property var modelData

                width: column.width
                height: 30
                spacing: Spacing.sm

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 8
                    height: 8
                    radius: 4
                    color: modelData.available ? Theme.success : Theme.textMuted
                    opacity: modelData.available ? 1 : 0.5
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 160
                    text: modelData.label
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    elide: Text.ElideRight
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 200
                    text: modelData.detail
                    color: modelData.available ? Theme.textMuted : Theme.warning
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    elide: Text.ElideRight
                }
            }
        }

        Text {
            width: parent.width
            text: qsTr("Ayar dosyası: %1").arg(SettingsStore.configPath)
            color: Theme.textMuted
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            wrapMode: Text.WrapAnywhere
        }
    }
}
