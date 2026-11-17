import QtQuick

// TagOS switch (used for Wi-Fi, Bluetooth, night light, ...).
Item {
    id: root

    property bool checked: false

    signal toggled(bool checked)

    implicitWidth: 42
    implicitHeight: 24
    opacity: enabled ? 1.0 : 0.4

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? Theme.accent : Theme.surfaceHover
        border.width: 1
        border.color: root.checked ? Theme.accentHover : Theme.border

        Behavior on color { ColorAnimation { duration: Motion.fast } }
    }

    Rectangle {
        id: knob
        width: 18
        height: 18
        radius: width / 2
        y: (parent.height - height) / 2
        x: root.checked ? parent.width - width - 3 : 3
        color: root.checked ? Theme.accentInk : Theme.textDim

        Behavior on x { NumberAnimation { duration: Motion.fast; easing.type: Motion.easing } }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (!root.enabled)
                return;
            root.checked = !root.checked;
            root.toggled(root.checked);
        }
    }
}
