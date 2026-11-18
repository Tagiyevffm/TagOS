import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.Control 1.0

// Ağ: NetworkManager durumu. Bağlantı düzenleme gerçek nm-connection-editor'e
// devredilir; TagOS ağ yığınını yeniden yazmaz.
Item {
    id: page

    property var control: SystemControl

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    function openConnectionEditor() {
        if (!AppCatalog.launchCommand("nm-connection-editor"))
            status.text = qsTr("nm-connection-editor bulunamadı (paket: network-manager-applet)");
    }

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Ağ")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Kablosuz ağ NetworkManager tarafından yönetilir.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Item { width: 1; height: Spacing.xs }

        SettingRow {
            title: qsTr("Kablosuz ağ")
            description: page.control && page.control.wifiAvailable
                         ? (page.control.wifiEnabled ? qsTr("Açık") : qsTr("Kapalı"))
                         : qsTr("nmcli bulunamadı — NetworkManager kurulu değil")
            icon: "wifi"

            TagToggle {
                checked: page.control ? page.control.wifiEnabled : false
                enabled: page.control ? page.control.wifiAvailable : false
                onToggled: checked => {
                    if (page.control)
                        page.control.setWifiEnabled(checked);
                }
            }
        }

        SettingRow {
            title: qsTr("Bağlı ağ")
            description: page.control && page.control.wifiConnected
                         ? qsTr("%1 · sinyal %2%").arg(page.control.wifiSsid).arg(page.control.wifiStrength)
                         : qsTr("Bağlı değil")
            icon: "globe"

            Text {
                text: page.control && page.control.wifiConnected ? page.control.wifiSecurity : ""
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Bağlantıları düzenle")
            description: qsTr("Gelişmiş ayarlar için nm-connection-editor açılır")
            icon: "wrench"

            TagButton {
                text: qsTr("Aç")
                variant: TagButton.Tonal
                onClicked: page.openConnectionEditor()
            }
        }

        Text {
            id: status
            width: parent.width
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }

        Item {
            width: parent.width
            height: !page.control || !page.control.wifiAvailable ? 150 : 0

            TagEmptyState {
                anchors.fill: parent
                visible: !page.control || !page.control.wifiAvailable
                icon: "wifi-off"
                title: qsTr("Ağ servisi bulunamadı")
                message: qsTr("Bu sistemde nmcli yok. Canlı TagOS imajında NetworkManager kurulu gelir; bu ekran gerçek durumu gösterir.")
            }
        }
    }
}
