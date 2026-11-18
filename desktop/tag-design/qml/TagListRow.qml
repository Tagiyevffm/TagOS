import QtQuick

// Icon + primary/secondary text + optional trailing accessory row.
// The single list building block for the launcher, TagSettings and TagFiles.
Item {
    id: root

    property string icon: ""
    property string title: ""
    property string subtitle: ""
    property string trailing: ""
    property bool selected: false
    property bool interactive: true

    default property alias accessories: accessoryRow.data

    signal clicked()
    signal doubleClicked()

    implicitHeight: 48
    implicitWidth: 240

    Rectangle {
        anchors.fill: parent
        radius: Radius.small
        color: root.selected ? Theme.accentSoft : (hover.containsMouse && root.interactive ? Theme.surfaceHover : "transparent")
        border.width: root.selected ? 1 : 0
        border.color: Theme.accent

        Behavior on color { ColorAnimation { duration: Motion.fast } }
    }

    Row {
        id: content
        anchors.fill: parent
        anchors.leftMargin: Spacing.md
        anchors.rightMargin: Spacing.md
        spacing: Spacing.md

        TagIcon {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon.length > 0
            name: root.icon
            size: 18
            opacity: root.selected ? 1.0 : 0.8
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - (root.icon.length > 0 ? 18 + Spacing.md : 0)
                    - (root.trailing.length > 0 ? trailingText.implicitWidth + Spacing.md : 0)
                    - accessoryRow.implicitWidth
            spacing: 1

            Text {
                width: parent.width
                text: root.title
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.body
                font.weight: root.selected ? Typography.semiBold : Typography.regular
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
            }
        }

        Row {
            id: accessoryRow
            anchors.verticalCenter: parent.verticalCenter
            spacing: Spacing.xs
        }

        Text {
            id: trailingText
            anchors.verticalCenter: parent.verticalCenter
            text: root.trailing
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        onDoubleClicked: root.doubleClicked()
    }
}
