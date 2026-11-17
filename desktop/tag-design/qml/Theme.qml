pragma Singleton
import QtQuick

// TagOS colour + identity tokens ("Aurora" palette).
//
// Flat properties on purpose: qmllint and qmltc can introspect them, and every
// surface reads the same values. Components must never hardcode a colour.
QtObject {
    // surfaces
    readonly property color bg: "#0E0B09"
    readonly property color bgElevated: "#14100E"
    readonly property color surface: "#1B1512"
    readonly property color surfaceAlt: "#221A16"
    readonly property color surfaceHover: "#2B211B"
    readonly property color border: "#332A24"
    readonly property color borderStrong: "#4A3D33"
    readonly property color shadow: "#B3000000"

    // accent
    readonly property color accent: "#F5931F"
    readonly property color accentHover: "#FFA93C"
    readonly property color accentPressed: "#D97C12"
    readonly property color accentSoft: "#26F5931F"
    readonly property color accentInk: "#161009"

    // text
    readonly property color text: "#F6EFE8"
    readonly property color textDim: "#A99C93"
    readonly property color textMuted: "#6E635C"

    // status
    readonly property color success: "#3FBF7F"
    readonly property color warning: "#F5C542"
    readonly property color danger: "#E5484D"
    readonly property color info: "#5B8DEF"

    // identity
    readonly property string version: "0.1.0"
    readonly property string codename: "Aurora"
    readonly property string tagline: qsTr("Dream. Code. Create.")
    readonly property string wallpaper: "qrc:/qt/qml/TagOS/Design/wallpapers/aurora-dawn.svg"

    function icon(name) {
        return "qrc:/qt/qml/TagOS/Design/icons/" + name + ".svg";
    }
}
