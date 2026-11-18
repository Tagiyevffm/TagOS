import QtQuick
import QtQuick.Controls

// Square, icon-only action button used by panels and app headers.
Item {
    id: root

    property string icon: "grid"
    property int size: 32
    property int iconSize: 18
    property bool checked: false
    property bool danger: false
    property string tooltip: ""
    property real opacityWhenIdle: 0.72

    signal clicked()

    implicitWidth: size
    implicitHeight: size
    opacity: enabled ? 1.0 : 0.35

    Rectangle {
        anchors.fill: parent
        radius: Radius.small
        color: root.danger && hover.containsMouse
               ? Theme.danger
               : (root.checked ? Theme.accentSoft : (hover.containsMouse ? Theme.surfaceHover : "transparent"))
        border.width: root.checked ? 1 : 0
        border.color: Theme.accent

        Behavior on color { ColorAnimation { duration: Motion.fast } }
    }

    TagIcon {
        anchors.centerIn: parent
        name: root.icon
        size: root.iconSize
        opacity: root.checked ? 1.0 : (hover.containsMouse ? 1.0 : root.opacityWhenIdle)
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: root.tooltip.length > 0 && hover.containsMouse
    ToolTip.text: root.tooltip
    ToolTip.delay: 550
}
