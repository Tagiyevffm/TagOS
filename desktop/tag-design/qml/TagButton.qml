import QtQuick

// TagOS button. One component, four variants — apps never re-style buttons.
Item {
    id: root

    enum Variant { Primary, Tonal, Ghost, Danger }

    property string text: ""
    property string icon: ""
    property int variant: TagButton.Primary
    property bool hanging: false
    property int horizontalPadding: Spacing.lg
    property int verticalPadding: Spacing.sm

    signal clicked()

    implicitWidth: row.implicitWidth + horizontalPadding * 2
    implicitHeight: Math.max(36, row.implicitHeight + verticalPadding * 2)
    opacity: enabled ? 1.0 : 0.45

    Rectangle {
        id: background
        anchors.fill: parent
        radius: Radius.small
        color: root.backgroundColor()
        border.width: root.variant === TagButton.Ghost ? 0 : 1
        border.color: root.variant === TagButton.Ghost
                      ? "transparent"
                      : (root.variant === TagButton.Primary ? Theme.accentHover : Theme.border)

        Behavior on color { ColorAnimation { duration: Motion.fast } }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Spacing.sm

        TagIcon {
            visible: root.icon.length > 0
            name: root.icon
            size: 18
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: root.text.length > 0
            text: root.text
            anchors.verticalCenter: parent.verticalCenter
            font.family: Typography.family
            font.pixelSize: Typography.body
            font.weight: Typography.medium
            color: root.foregroundColor()
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    function backgroundColor() {
        switch (variant) {
        case TagButton.Primary:
            return area.pressed ? Theme.accentPressed
                                : (area.containsMouse ? Theme.accentHover : Theme.accent);
        case TagButton.Tonal:
            return area.containsMouse ? Theme.surfaceHover : Theme.surfaceAlt;
        case TagButton.Danger:
            return area.containsMouse ? "#F06A6E" : Theme.danger;
        case TagButton.Ghost:
        default:
            return area.containsMouse ? Theme.surfaceAlt : "transparent";
        }
    }

    function foregroundColor() {
        switch (variant) {
        case TagButton.Primary:
            return Theme.accentInk;
        case TagButton.Danger:
            return "#1A0D0D";
        case TagButton.Tonal:
            return Theme.text;
        case TagButton.Ghost:
        default:
            return area.containsMouse ? Theme.text : Theme.textDim;
        }
    }
}
