import QtQuick

// "Nothing here yet" state with an icon, a title and an explanation. Used
// instead of an empty rectangle so a missing backend is visible and honest.
Column {
    id: root

    property string icon: "search"
    property string title: ""
    property string message: ""

    spacing: Spacing.md
    width: 260

    TagIcon {
        anchors.horizontalCenter: parent.horizontalCenter
        name: root.icon
        size: 34
        opacity: 0.5
    }

    Text {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        color: Theme.textDim
        font.family: Typography.family
        font.pixelSize: Typography.body
        font.weight: Typography.semiBold
        wrapMode: Text.WordWrap
    }

    Text {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        visible: root.message.length > 0
        text: root.message
        color: Theme.textMuted
        font.family: Typography.family
        font.pixelSize: Typography.caption
        wrapMode: Text.WordWrap
        lineHeight: 1.25
    }
}
