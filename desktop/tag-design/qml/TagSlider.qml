import QtQuick

// TagOS slider (brightness, volume, ...). Value is 0..1.
Item {
    id: root

    property real value: 0.5
    property real from: 0
    property real to: 1
    property color trackColor: Theme.surfaceHover
    property color progressColor: Theme.accent
    property int trackHeight: 4

    signal moved(real value)

    implicitWidth: 200
    implicitHeight: 24
    opacity: enabled ? 1.0 : 0.4

    readonly property real ratio: (to - from) === 0 ? 0 : Math.max(0, Math.min(1, (value - from) / (to - from)))

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: root.trackHeight
        radius: height / 2
        color: root.trackColor
    }

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: track.width * root.ratio
        height: root.trackHeight
        radius: height / 2
        color: root.progressColor
    }

    Rectangle {
        id: knob
        width: 14
        height: 14
        radius: width / 2
        anchors.verticalCenter: parent.verticalCenter
        x: Math.max(0, Math.min(parent.width - width, parent.width * root.ratio - width / 2))
        color: Theme.text
        border.width: 2
        border.color: Theme.accent
    }

    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: -6
        anchors.rightMargin: -6
        cursorShape: Qt.PointingHandCursor
        onPositionChanged: (mouse) => {
            const clamped = Math.max(0, Math.min(1, mouse.x / width));
            root.value = root.from + clamped * (root.to - root.from);
            root.moved(root.value);
        }
        onPressed: (mouse) => {
            const clamped = Math.max(0, Math.min(1, mouse.x / width));
            root.value = root.from + clamped * (root.to - root.from);
            root.moved(root.value);
        }
    }
}
