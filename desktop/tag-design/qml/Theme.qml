pragma Singleton
import QtQuick
import TagOS.Config 1.0

// TagOS design tokens — "Aurora" palette, dark and light.
//
// Every surface reads colours from here; nothing hardcodes a hex value. The
// active palette follows SettingsStore (dark / light / auto), so changing the
// theme in TagSettings or TagCenter repaints the entire desktop through normal
// QML bindings — no restart, no manual re-theming.
QtObject {
    id: theme

    readonly property bool dark: SettingsStore.darkMode
    readonly property string modeName: dark ? "dark" : "light"

    // --- surfaces ---------------------------------------------------------
    readonly property color bg: dark ? "#090A0C" : "#F4F5F7"
    readonly property color bgElevated: dark ? "#0F1115" : "#FFFFFF"
    readonly property color surface: dark ? "#111318" : "#FFFFFF"
    readonly property color card: dark ? "#17191F" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#1C1F26" : "#F0F1F4"
    readonly property color surfaceHover: dark ? "#23272F" : "#E8EAF0"
    readonly property color overlay: dark ? "#CC090A0C" : "#B3F4F5F7"
    readonly property color border: dark ? "#24272F" : "#E1E4EA"
    readonly property color borderStrong: dark ? "#343944" : "#C7CBD4"
    readonly property color shadow: dark ? "#99000000" : "#1F1A1D24"

    // --- accent -----------------------------------------------------------
    readonly property color accent: dark ? "#FF9D24" : "#F08A0E"
    readonly property color accentHover: dark ? "#FFB04D" : "#FF9D24"
    readonly property color accentPressed: dark ? "#E3851A" : "#C97400"
    readonly property color accentSoft: dark ? "#26FF9D24" : "#1AF08A0E"
    readonly property color accentInk: "#15100A"
    readonly property color secondary: dark ? "#F5B84B" : "#B4690A"

    // --- text -------------------------------------------------------------
    readonly property color text: dark ? "#F4F4F5" : "#14161B"
    readonly property color textDim: dark ? "#C3C5CC" : "#3F444E"
    readonly property color textMuted: dark ? "#92949D" : "#6B7280"

    // --- status -----------------------------------------------------------
    readonly property color success: dark ? "#4ADE80" : "#15803D"
    readonly property color warning: dark ? "#F5B84B" : "#B45309"
    readonly property color danger: dark ? "#F2555A" : "#C0292E"
    readonly property color info: dark ? "#5B9DFF" : "#1D4ED8"

    // --- translucent surfaces (panel, dock, floating chrome) ---------------
    // Kept here so no QML file outside this one spells a colour literal.
    readonly property color surfacePanel: dark ? "#D90B0D11" : "#F2FFFFFF"
    readonly property color surfaceDock: dark ? "#E6111318" : "#F7FFFFFF"
    readonly property color dangerHover: dark ? "#F06A6E" : "#A8232A"
    readonly property color dangerOnText: dark ? "#1A0D0D" : "#FFFFFF"
    /// Bottom gradient applied over the wallpaper so the clock stays legible.
    readonly property color wallpaperShade: dark ? "#B3090A0C" : "#66FFFFFF"

    // --- identity ---------------------------------------------------------
    readonly property string version: "0.2.0"
    readonly property string codename: "Aurora"
    readonly property string tagline: qsTr("Dream. Code. Create.")
    readonly property string defaultWallpaperDark: "qrc:/qt/qml/TagOS/Design/wallpapers/aurora-dawn.svg"
    readonly property string defaultWallpaperLight: "qrc:/qt/qml/TagOS/Design/wallpapers/aurora-day.svg"
    readonly property string wallpaper: SettingsStore.wallpaper.length > 0
                                        ? SettingsStore.wallpaper
                                        : (dark ? defaultWallpaperDark : defaultWallpaperLight)

    /// Icon URL for the active theme (icons ship as static dark/light sets).
    function icon(name) {
        const set = dark ? "icons" : "icons/light";
        return "qrc:/qt/qml/TagOS/Design/" + set + "/" + name + ".svg";
    }

    /// Shared easing/duration helpers so every component animates identically.
    function duration(ms) {
        return SettingsStore.reducedMotion ? 0 : ms;
    }
}
