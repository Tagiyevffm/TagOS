import QtQuick
import TagOS.Design 1.0

// Desktop backdrop: theme wallpaper plus a readability veil. The wallpaper is
// a design-system asset, so a user-set photo only changes Theme.wallpaper.
Item {
    id: root

    property real veil: 0.55

    Image {
        anchors.fill: parent
        source: Theme.wallpaper
        fillMode: Image.PreserveAspectCrop
        asynchronous: false
        cache: true
        smooth: true
    }

    // Bottom-anchored gradient keeps the dock and desktop text legible on any
    // wallpaper without hiding the artwork.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop {
                position: 0.55
                color: Qt.rgba(0, 0, 0, Theme.dark ? 0.10 : 0.04)
            }
            GradientStop { position: 1.0; color: Theme.wallpaperShade }
        }
        opacity: root.veil
    }

    // Subtle accent glow behind the hero widget.
    Rectangle {
        width: parent.width * 0.45
        height: width
        radius: width / 2
        x: -width * 0.2
        y: parent.height - height * 0.75
        color: Theme.accentSoft
        opacity: Theme.dark ? 0.30 : 0.18
    }
}
