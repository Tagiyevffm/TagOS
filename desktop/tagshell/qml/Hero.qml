import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0
import TagOS.Flow 1.0
import TagOS.System 1.0

// Desktop clock / greeting widget. Everything it shows is live: time, uptime,
// CPU and memory come from TagFlow, the version from the build.
Item {
    id: root

    property var flow: null
    property string greeting: ""
    property string dateText: ""

    implicitWidth: 320
    implicitHeight: content.implicitHeight
    width: implicitWidth
    height: implicitHeight

    readonly property var monthNames: ["Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
                                       "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık"]
    readonly property var dayNames: ["Pazar", "Pazartesi", "Salı", "Çarşamba", "Perşembe",
                                     "Cuma", "Cumartesi"]

    function updateClock() {
        const now = new Date();
        const hour = now.getHours();
        root.greeting = hour < 6 ? qsTr("İyi geceler")
                      : hour < 12 ? qsTr("Günaydın")
                      : hour < 18 ? qsTr("İyi günler")
                      : qsTr("İyi akşamlar");
        root.dateText = root.dayNames[now.getDay()] + ", " + now.getDate() + " "
                + root.monthNames[now.getMonth()];
        clock.text = (SettingsStore.showSeconds
                      ? Qt.formatDateTime(now, "HH:mm:ss")
                      : Qt.formatDateTime(now, "HH:mm"));
    }

    Component.onCompleted: updateClock()

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.updateClock()
    }

    Column {
        id: content
        width: parent.width
        spacing: Spacing.xs

        Text {
            text: root.greeting
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.letterSpacing: 0.4
        }

        Text {
            id: clock
            text: Qt.formatDateTime(new Date(), "HH:mm")
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: 68
            font.weight: Typography.regular
            font.letterSpacing: -2
        }

        Text {
            text: root.dateText
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.body
        }

        Item { width: 1; height: Spacing.sm }

        Row {
            spacing: Spacing.sm

            Rectangle {
                height: 24
                width: versionRow.implicitWidth + Spacing.md * 2
                radius: Radius.pill
                color: Theme.accentSoft
                border.width: 1
                border.color: Theme.accent

                Row {
                    id: versionRow
                    anchors.centerIn: parent
                    spacing: Spacing.xs

                    TagIcon { anchors.verticalCenter: parent.verticalCenter; name: "tagos-logo"; size: 13 }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "TagOS " + Theme.version + " · " + Theme.codename
                        color: Theme.text
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.medium
                    }
                }
            }

            Rectangle {
                height: 24
                width: uptimeRow.implicitWidth + Spacing.md * 2
                radius: Radius.pill
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.border

                Row {
                    id: uptimeRow
                    anchors.centerIn: parent
                    spacing: Spacing.xs

                    TagIcon { anchors.verticalCenter: parent.verticalCenter; name: "clock"; size: 13; opacity: 0.7 }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.flow && root.flow.valid
                              ? qsTr("%1 açık").arg(root.flow.uptimeText)
                              : qsTr("Ölçüm yok")
                        color: Theme.textDim
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }
                }
            }
        }
    }
}
