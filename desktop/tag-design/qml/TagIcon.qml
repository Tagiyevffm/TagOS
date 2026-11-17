import QtQuick

// Monochrome TagOS icon (theme/icons/*.svg compiled into this module).
Item {
    id: root

    property string name: "grid"
    property int size: 22
    /// Optional tint: icons in the theme are drawn for dark surfaces.
    property real dim: 1.0

    implicitWidth: size
    implicitHeight: size
    opacity: dim

    Image {
        anchors.fill: parent
        source: Theme.icon(root.name)
        sourceSize.width: Math.round(root.size * 2)
        sourceSize.height: Math.round(root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: false
    }
}
