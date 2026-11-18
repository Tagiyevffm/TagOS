import QtQuick

// Monochrome TagOS icon (theme/icons/*.svg compiled into this module).
Item {
    id: root

    property string name: "grid"
    /// Shown when the requested icon is not part of the TagOS icon set (foreign
    /// .desktop files reference icon-theme names we do not ship).
    property string fallback: "app"
    property int size: 22
    /// Optional dimming: icons are drawn for the active theme's text colour.
    property real dim: 1.0

    implicitWidth: size
    implicitHeight: size
    opacity: dim

    property string resolvedSource: Theme.icon(root.name.length > 0 ? root.name : root.fallback)

    onNameChanged: resolvedSource = Theme.icon(root.name.length > 0 ? root.name : root.fallback)

    Image {
        anchors.fill: parent
        source: root.resolvedSource
        sourceSize.width: Math.round(root.size * 2)
        sourceSize.height: Math.round(root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: false

        onStatusChanged: {
            if (status === Image.Error && root.resolvedSource !== Theme.icon(root.fallback))
                root.resolvedSource = Theme.icon(root.fallback);
        }
    }
}
