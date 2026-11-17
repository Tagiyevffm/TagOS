pragma Singleton
import QtQuick

// TagOS motion language: short, consistent, never decorative.
QtObject {
    readonly property int fast: 110
    readonly property int normal: 190
    readonly property int slow: 320
    readonly property int easing: Easing.OutCubic
}
