import QtQuick
import TagOS.Design 1.0
import TagOS.Flow 1.0
import TagOS.System 1.0

// TagAbout — sürüm, çekirdek, donanım ve lisanslar. Tüm değerler oturumdan okunur.
TagAppWindow {
    id: root

    iconName: "tagos-logo"
    titleText: qsTr("TagOS Hakkında")
    width: 720
    height: 640

    FlowClient { id: flow }

    Column {
        anchors.fill: parent
        anchors.margins: Spacing.xxl
        spacing: Spacing.lg

        Row {
            spacing: Spacing.lg

            Rectangle {
                width: 88
                height: 88
                radius: Radius.large
                color: Theme.accentSoft
                border.width: 1
                border.color: Theme.accent

                TagIcon {
                    anchors.centerIn: parent
                    name: "tagos-logo"
                    size: 48
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Spacing.xs

                Text {
                    text: "TagOS"
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.headline
                    font.weight: Typography.bold
                    font.letterSpacing: 0.5
                }

                Text {
                    text: qsTr("Sürüm %1 · %2").arg(Theme.version).arg(Theme.codename)
                    color: Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                }

                Text {
                    text: Theme.tagline
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Grid {
            width: parent.width
            columns: 2
            columnSpacing: Spacing.xl
            rowSpacing: Spacing.md

            Text { text: qsTr("Temel dağıtım"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: SessionInfo.prettyOsName; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body; elide: Text.ElideRight }

            Text { text: qsTr("Çekirdek"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: flow.kernelVersion.length > 0 ? flow.kernelVersion : SessionInfo.kernelVersion; color: Theme.text; font.family: Typography.monoFamily; font.pixelSize: Typography.body }

            Text { text: qsTr("Mimari"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: SessionInfo.architecture; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

            Text { text: qsTr("İşlemci"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: SessionInfo.cpuModel; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body; elide: Text.ElideRight }

            Text { text: qsTr("Bellek"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: flow.valid ? flow.memoryText : SessionInfo.totalMemoryText; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

            Text { text: qsTr("Masaüstü"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: "labwc + TagShell · " + SessionInfo.sessionType; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

            Text { text: qsTr("Ölçüm kanalı"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: flow.transportLabel; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

            Text { text: qsTr("Derleme"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
            Text { width: 360; text: SessionInfo.buildType + " · Qt " + SessionInfo.qtVersion; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Text {
            text: qsTr("Lisanslar")
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.weight: Typography.medium
        }

        Column {
            width: parent.width
            spacing: Spacing.xs

            Repeater {
                model: [
                    { component: qsTr("TagOS bileşenleri"), license: "GPL-3.0-or-later" },
                    { component: "Qt 6", license: "LGPL-3.0 / GPL-3.0" },
                    { component: "labwc (Wayland compositor)", license: "GPL-2.0-or-later" },
                    { component: "systemd", license: "LGPL-2.1-or-later" },
                    { component: "Arch Linux paketleri", license: qsTr("paket başına değişir") }
                ]

                delegate: Row {
                    required property var modelData
                    width: parent.width
                    height: 26
                    spacing: Spacing.md

                    Text {
                        width: 320
                        text: modelData.component
                        color: Theme.text
                        font.family: Typography.family
                        font.pixelSize: Typography.body
                        elide: Text.ElideRight
                    }

                    Text {
                        text: modelData.license
                        color: Theme.textMuted
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                    }
                }
            }
        }

        Item { width: 1; height: 1 }

        Text {
            width: parent.width
            text: qsTr("TagOS bağımsız bir dağıtımdır; Arch Linux tabanını kullanır ancak kendi masaüstü, servisleri ve uygulamalarıyla gelir.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }
    }
}
