import QtQuick
import TagOS.Design 1.0

// Toast stack for the newest notifications, bottom-right above the dock.
Item {
    id: root

    property var notifications: null
    property int visibleCount: 3

    implicitWidth: 340
    implicitHeight: column.implicitHeight
    width: implicitWidth
    height: implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: Spacing.sm

        Repeater {
            model: root.notifications ? root.notifications.model : null

            delegate: Item {
                id: toast
                required property int index
                required property string title
                required property string body
                required property string icon

                width: column.width
                height: index < root.visibleCount ? 64 : 0
                visible: index < root.visibleCount
                opacity: visible ? 1 : 0

                Behavior on opacity { NumberAnimation { duration: Motion.normal } }

                Rectangle {
                    anchors.fill: parent
                    radius: Radius.medium
                    color: Theme.card
                    border.width: 1
                    border.color: Theme.borderStrong
                }

                Row {
                    anchors.fill: parent
                    anchors.margins: Spacing.md
                    spacing: Spacing.md

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 28
                        radius: Radius.small
                        color: Theme.accentSoft

                        TagIcon {
                            anchors.centerIn: parent
                            name: toast.icon && toast.icon.length > 0 ? toast.icon : "bell"
                            size: 15
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 60
                        spacing: 2

                        Text {
                            width: parent.width
                            text: toast.title
                            color: Theme.text
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                            font.weight: Typography.semiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: toast.body
                            color: Theme.textMuted
                            font.family: Typography.family
                            font.pixelSize: Typography.caption
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                        }
                    }

                    TagIconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon: "close"
                        size: 22
                        iconSize: 12
                        onClicked: if (root.notifications) root.notifications.dismiss(toast.index)
                    }
                }

                Timer {
                    interval: 6000
                    running: toast.visible
                    onTriggered: if (root.notifications) root.notifications.dismiss(toast.index)
                }
            }
        }
    }
}
