import QtQuick
import TagOS.Design 1.0

// One settings row: label + description on the left, control on the right.
Item {
    id: root

    property string title: ""
    property string description: ""
    property string icon: ""

    default property alias control: slot.data

    implicitHeight: Math.max(48, rowContent.implicitHeight + Spacing.md)
    width: parent ? parent.width : 320

    Row {
        id: rowContent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Spacing.md

        TagIcon {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon.length > 0
            name: root.icon
            size: 17
            opacity: 0.75
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - slot.width - (root.icon.length > 0 ? 29 : 0) - Spacing.md * 2
            spacing: 2

            Text {
                width: parent.width
                text: root.title
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.body
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: root.description.length > 0
                text: root.description
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                wrapMode: Text.WordWrap
                lineHeight: 1.2
            }
        }

        Item {
            id: slot
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
        opacity: 0.6
    }
}
