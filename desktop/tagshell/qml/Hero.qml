import QtQuick
import TagOS.Design 1.0

// Centre of the board: identity + the universal launcher entry.
Item {
    id: root

    signal searchRequested()

    Column {
        anchors.centerIn: parent
        spacing: Spacing.lg

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Spacing.lg

            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: Theme.icon("tagos-logo")
                sourceSize.width: 96
                sourceSize.height: 88
                width: 84
                height: 76
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Text {
                    text: "Tag"
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: 72
                    font.weight: Typography.bold
                    font.letterSpacing: -2
                }

                Text {
                    text: "OS"
                    color: Theme.accent
                    font.family: Typography.family
                    font.pixelSize: 72
                    font.weight: Typography.bold
                    font.letterSpacing: -2
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Theme.tagline
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.letterSpacing: 3
        }

        Item { width: 1; height: Spacing.sm }

        Rectangle {
            id: searchField
            anchors.horizontalCenter: parent.horizontalCenter
            width: 430
            height: 44
            radius: Radius.pill
            color: Theme.bgElevated
            border.width: 1
            border.color: area.containsMouse ? Theme.accent : Theme.border

            Behavior on border.color { ColorAnimation { duration: Motion.fast } }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Spacing.lg
                anchors.verticalCenter: parent.verticalCenter
                spacing: Spacing.md

                TagIcon { name: "search"; size: 17; dim: 0.85; anchors.verticalCenter: parent.verticalCenter }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Her şeyi ara…")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                }
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.searchRequested()
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Dream. Code. Create.  ·  TagOS %1 (%2)").arg(Theme.version).arg(Theme.codename)
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }
    }
}
