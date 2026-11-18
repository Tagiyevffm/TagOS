import QtQuick
import ".."
import TagOS.Design 1.0
import TagOS.System 1.0

// Kullanıcı: oturum bilgileri (salt okunur).
Item {
    id: page

    implicitHeight: column.implicitHeight
    width: parent ? parent.width : 600

    Column {
        id: column
        width: parent.width
        spacing: Spacing.md

        Text {
            text: qsTr("Kullanıcı")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.headline
            font.weight: Typography.semiBold
        }

        Text {
            text: qsTr("Hesap yönetimi henüz TagOS'un kapsamında değil; bu sayfa oturumun gerçek bilgilerini gösterir.")
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
        }

        Item { width: 1; height: Spacing.sm }

        Row {
            spacing: Spacing.lg

            Rectangle {
                width: 72
                height: 72
                radius: width / 2
                color: Theme.accentSoft
                border.width: 1
                border.color: Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: SessionInfo.userName.length > 0
                          ? SessionInfo.userName.substring(0, 2).toUpperCase() : "?"
                    color: Theme.accent
                    font.family: Typography.family
                    font.pixelSize: Typography.title
                    font.weight: Typography.bold
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: SessionInfo.userName
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.title
                    font.weight: Typography.semiBold
                }

                Text {
                    text: SessionInfo.hostName
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }

        Item { width: 1; height: Spacing.sm }

        SettingRow {
            title: qsTr("Ev dizini")
            icon: "home"

            Text {
                text: SessionInfo.homePath
                color: Theme.textDim
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Kabuk")
            icon: "terminal"

            Text {
                text: SessionInfo.shellPath
                color: Theme.textDim
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Oturum türü")
            icon: "layers"

            Text {
                text: SessionInfo.sessionType + " · " + SessionInfo.desktopName
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }

        SettingRow {
            title: qsTr("Dil / yerel ayar")
            icon: "globe"

            Text {
                text: SessionInfo.localeName
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }
    }
}
