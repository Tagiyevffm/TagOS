pragma Singleton
import QtQuick

// TagOS typography scale and weights.
QtObject {
    readonly property string family: "Inter, Noto Sans, DejaVu Sans, sans-serif"
    readonly property string monoFamily: "JetBrains Mono, Noto Sans Mono, monospace"

    readonly property int caption: 11
    readonly property int small: 12
    readonly property int body: 13
    readonly property int subtitle: 15
    readonly property int title: 18
    readonly property int headline: 24
    readonly property int display: 34
    readonly property int hero: 64

    readonly property int regular: Font.Normal
    readonly property int medium: Font.Medium
    readonly property int bold: Font.DemiBold
    readonly property int semiBold: Font.DemiBold
}
