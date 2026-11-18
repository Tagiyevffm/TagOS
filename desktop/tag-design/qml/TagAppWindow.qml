import QtQuick

// Standard TagOS application window chrome.
//
// Every TagOS application (TagFiles, TagSettings, TagMonitor, TagTerminal,
// TagAbout) opens through this component so window controls, spacing and the
// header look identical, and a change to the chrome is a one-file change.
Window {
    id: root

    property string iconName: "app"
    property string titleText: "TagOS"
    property string subtitle: ""
    property bool showHeader: true
    property int headerHeight: 46

    default property alias content: body.data

    width: 960
    height: 640
    minimumWidth: 520
    minimumHeight: 380
    color: Theme.bg
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: titleText + " — TagOS"

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.showHeader ? root.headerHeight : 0
        visible: root.showHeader
        color: Theme.surface
        border.width: 0

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: Spacing.lg
            anchors.verticalCenter: parent.verticalCenter
            spacing: Spacing.md

            TagIcon {
                anchors.verticalCenter: parent.verticalCenter
                name: root.iconName
                size: 18
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Text {
                    text: root.titleText
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    font.weight: Typography.semiBold
                }

                Text {
                    visible: root.subtitle.length > 0
                    text: root.subtitle
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: Spacing.sm
            anchors.verticalCenter: parent.verticalCenter
            spacing: Spacing.xxs

            TagIconButton {
                icon: "minimize"
                iconSize: 16
                size: 30
                tooltip: qsTr("Küçült")
                onClicked: root.showMinimized()
            }

            TagIconButton {
                icon: "close"
                iconSize: 16
                size: 30
                danger: true
                tooltip: qsTr("Kapat")
                onClicked: root.close()
            }
        }

        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: 80
            acceptedButtons: Qt.LeftButton
            onPressed: root.startSystemMove()
            onDoubleClicked: root.visibility = root.visibility === Window.Maximized
                                     ? Window.Windowed : Window.Maximized
        }
    }

    Item {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        clip: true
    }
}
