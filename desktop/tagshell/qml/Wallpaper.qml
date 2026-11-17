import QtQuick
import TagOS.Design 1.0

// Aurora wallpaper with a soft vignette so panels stay readable.
Item {
    id: root

    Image {
        anchors.fill: parent
        source: Theme.wallpaper
        fillMode: Image.PreserveAspectCrop
        asynchronous: false
        smooth: true
        cache: true
    }

    // Bottom and top scrims: panels get contrast without dimming the artwork.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#B3000000" }
            GradientStop { position: 0.22; color: "#33000000" }
            GradientStop { position: 0.62; color: "#11000000" }
            GradientStop { position: 1.0; color: "#CC000000" }
        }
    }
}
