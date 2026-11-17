import QtQuick
import TagOS.Design 1.0

// Label/value row used by the monitor's detail list.
Row {
    id: root

    property string label: ""
    property string value: ""

    spacing: Spacing.sm

    Text {
        text: root.label
        color: Theme.textMuted
        font.family: Typography.family
        font.pixelSize: Typography.body
        width: 180
    }

    Text {
        text: root.value
        color: Theme.textDim
        font.family: Typography.family
        font.pixelSize: Typography.body
        elide: Text.ElideRight
        width: root.width - 200
    }
}
