import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.Config 1.0

// Görünüm: tema, duvar kağıdı, yerleşim ölçüleri, hareket.
Item {
    id: page

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Görünüm")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Tüm TagOS yüzeyleri bu ayarlardan beslenir; değişiklik anında uygulanır.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Item { width: 1; height: Spacing.xs }

        SettingRow {
            title: qsTr("Tema")
            description: qsTr("Koyu, açık veya sistem tercihini izle")
            icon: "palette"

            TagSegmented {
                options: [
                    { label: qsTr("Koyu"), value: "dark" },
                    { label: qsTr("Açık"), value: "light" },
                    { label: qsTr("Otomatik"), value: "auto" }
                ]
                value: SettingsStore.themeMode
                onSelected: value => SettingsStore.setThemeMode(value)
            }
        }

        SettingRow {
            title: qsTr("Duvar kağıdı")
            description: qsTr("TagOS duvar kağıtları veya kendi dosyanız")
            icon: "image"

            Row {
                spacing: Spacing.xs

                Repeater {
                    model: [
                        { label: qsTr("Aurora"), value: "" },
                        { label: qsTr("Gündüz"), value: Theme.defaultWallpaperLight }
                    ]

                    delegate: TagButton {
                        required property var modelData
                        text: modelData.label
                        variant: SettingsStore.wallpaper === modelData.value
                                 ? TagButton.Primary : TagButton.Tonal
                        onClicked: SettingsStore.setWallpaper(modelData.value)
                    }
                }
            }
        }

        SettingRow {
            title: qsTr("Panel yüksekliği")
            description: qsTr("Üst panelin yüksekliği (%1 px)").arg(SettingsStore.panelHeight)
            icon: "grid"

            TagSlider {
                width: 220
                from: 32
                to: 72
                value: SettingsStore.panelHeight
                onMoved: value => SettingsStore.setPanelHeight(Math.round(value))
            }
        }

        SettingRow {
            title: qsTr("Dock simge boyutu")
            description: qsTr("%1 px").arg(SettingsStore.dockIconSize)
            icon: "layers"

            TagSlider {
                width: 220
                from: 28
                to: 64
                value: SettingsStore.dockIconSize
                onMoved: value => SettingsStore.setDockIconSize(Math.round(value))
            }
        }

        SettingRow {
            title: qsTr("Dock'u otomatik gizle")
            description: qsTr("İmleç ekranın altına gelince görünür")
            icon: "arrow-down"

            TagToggle {
                checked: SettingsStore.dockAutoHide
                onToggled: checked => SettingsStore.setDockAutoHide(checked)
            }
        }

        SettingRow {
            title: qsTr("Saniyeleri göster")
            description: qsTr("Panel ve masaüstü saatinde saniye")
            icon: "clock"

            TagToggle {
                checked: SettingsStore.showSeconds
                onToggled: checked => SettingsStore.setShowSeconds(checked)
            }
        }

        SettingRow {
            title: qsTr("Hareketi azalt")
            description: qsTr("Animasyon sürelerini sıfırlar")
            icon: "speed"

            TagToggle {
                checked: SettingsStore.reducedMotion
                onToggled: checked => SettingsStore.setReducedMotion(checked)
            }
        }

        SettingRow {
            title: qsTr("Yazı boyutu")
            description: qsTr("%1 px · yeni pencerelerde geçerli").arg(SettingsStore.fontSize)
            icon: "edit"

            TagSlider {
                width: 220
                from: 10
                to: 18
                value: SettingsStore.fontSize
                onMoved: value => SettingsStore.setFontSize(Math.round(value))
            }
        }

        Rectangle {
            width: parent.width
            height: note.implicitHeight + Spacing.md * 2
            radius: Radius.small
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Text {
                id: note
                anchors.fill: parent
                anchors.margins: Spacing.md
                text: qsTr("Ayarlar %1 dosyasına yazılır ve atomik olarak kaydedilir.")
                      .arg(SettingsStore.configPath)
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                wrapMode: Text.WrapAnywhere
            }
        }
    }
}
