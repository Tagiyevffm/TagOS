pragma ComponentBehavior: Bound

import QtQuick

// Segmented control: 2–4 mutually exclusive options.
Item {
    id: root

    property var options: []          // [{ label: "...", value: "..." }]
    property string value: ""
    property int optionHeight: 30

    signal selected(string value)

    implicitHeight: optionHeight
    implicitWidth: row.implicitWidth + Spacing.xs * 2

    Rectangle {
        anchors.fill: parent
        radius: Radius.small
        color: Theme.surfaceAlt
        border.width: 1
        border.color: Theme.border
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Spacing.xs

        Repeater {
            model: root.options

            delegate: Item {
                id: segment
                required property var modelData

                width: Math.max(64, text.implicitWidth + Spacing.xl)
                height: root.optionHeight - 6

                Rectangle {
                    anchors.fill: parent
                    radius: Radius.small - 2
                    color: root.value === segment.modelData.value ? Theme.accent : "transparent"

                    Behavior on color { ColorAnimation { duration: Motion.fast } }
                }

                Text {
                    id: text
                    anchors.centerIn: parent
                    text: segment.modelData.label
                    color: root.value === segment.modelData.value ? Theme.accentInk : Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    font.weight: root.value === segment.modelData.value
                                 ? Typography.semiBold : Typography.regular
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.value === segment.modelData.value)
                            return;
                        root.value = segment.modelData.value;
                        root.selected(segment.modelData.value);
                    }
                }
            }
        }
    }
}
