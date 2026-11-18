import QtQuick

// Toggle pill: "Wi-Fi", "Bluetooth", "Gece ışığı"… Used by TagCenter.
Item {
    id: root

    property string icon: "grid"
    property string label: ""
    property string detail: ""
    property bool active: false
    property bool available: true
    property bool busy: false

    signal toggled()

    implicitWidth: Math.max(112, row.implicitWidth + Spacing.lg * 2)
    implicitHeight: 38
    opacity: available ? 1.0 : 0.45

    Rectangle {
        anchors.fill: parent
        radius: Radius.pill
        color: root.active ? Theme.accentSoft : Theme.surfaceAlt
        border.width: 1
        border.color: root.active ? Theme.accent : Theme.border

        Behavior on color { ColorAnimation { duration: Motion.fast } }
        Behavior on border.color { ColorAnimation { duration: Motion.fast } }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Spacing.sm

        TagIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 16
            opacity: root.active ? 1.0 : 0.75
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label
            color: root.active ? Theme.text : Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.small
            font.weight: root.active ? Typography.semiBold : Typography.regular
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.detail.length > 0
            text: root.detail
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption
            elide: Text.ElideRight
            width: Math.min(implicitWidth, 96)
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.available && !root.busy
        cursorShape: root.available ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.toggled()
    }
}
