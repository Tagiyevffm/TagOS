import QtQuick

// Rounded TagOS surface used by every panel, popup and app page.
Rectangle {
    id: root

    default property alias contentData: content.data

    property int padding: Spacing.lg
    property color surfaceColor: Theme.surface
    property color borderColor: Theme.border
    property int radiusValue: Radius.medium
    property bool elevated: true

    radius: radiusValue
    color: surfaceColor
    border.width: 1
    border.color: borderColor

    Item {
        id: content
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
