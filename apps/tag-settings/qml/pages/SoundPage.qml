import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.Control 1.0

// Ses: PipeWire varsayılan çıkışı (wpctl).
Item {
    id: page

    property var control: SystemControl

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Ses")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Varsayılan çıkış aygıtı PipeWire üzerinden denetlenir.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Item { width: 1; height: Spacing.xs }

        SettingRow {
            title: qsTr("Çıkış ses düzeyi")
            description: page.control && page.control.audioAvailable
                         ? qsTr("%1%").arg(page.control.volume)
                         : qsTr("wpctl bulunamadı")
            icon: "volume"

            TagSlider {
                width: 240
                value: page.control ? page.control.volume / 100 : 0
                enabled: page.control ? page.control.audioAvailable : false
                onMoved: value => {
                    if (page.control)
                        page.control.setVolume(Math.round(value * 100));
                }
            }
        }

        SettingRow {
            title: qsTr("Sessiz")
            description: qsTr("Çıkışı geçici olarak kapat")
            icon: "volume-off"

            TagToggle {
                checked: page.control ? page.control.muted : false
                enabled: page.control ? page.control.audioAvailable : false
                onToggled: checked => {
                    if (page.control)
                        page.control.setMuted(checked);
                }
            }
        }

        Item {
            width: parent.width
            height: 140

            TagEmptyState {
                anchors.fill: parent
                visible: !page.control || !page.control.audioAvailable
                icon: "volume-off"
                title: qsTr("Ses servisi yok")
                message: qsTr("PipeWire (wpctl) kurulu değil. Canlı imajda wireplumber ile birlikte gelir.")
            }
        }
    }
}
