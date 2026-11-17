pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0

// Top panel: identity, workspaces, launcher entry, status area and clock.
Item {
    id: root

    property int currentWorkspace: 0
    property string hostName: "tagos"
    property int notifications: 2
    property bool online: true
    property int batteryPercent: 87

    signal launcherRequested()
    signal controlCenterRequested()
    signal workspaceSelected(int index)

    implicitHeight: 56

    // --- identity + workspaces -------------------------------------------
    Row {
        id: leftCluster
        anchors.left: parent.left
        anchors.leftMargin: Spacing.xl
        anchors.verticalCenter: parent.verticalCenter
        spacing: Spacing.lg

        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: Theme.icon("tagos-logo")
            sourceSize.width: 28
            sourceSize.height: 28
            width: 26
            height: 26
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "TagOS"
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: Typography.subtitle
            font.weight: Typography.bold
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: Spacing.xs
            topPadding: 0

            Repeater {
                model: 4
                delegate: Rectangle {
                    id: pill
                    required property int index
                    width: 22
                    height: 22
                    radius: Radius.small
                    color: root.currentWorkspace === index ? Theme.accent : "transparent"
                    border.width: root.currentWorkspace === index ? 0 : 1
                    border.color: Theme.border

                    Text {
                        anchors.centerIn: parent
                        text: pill.index + 1
                        color: root.currentWorkspace === pill.index ? Theme.accentInk : Theme.textDim
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        font.weight: Typography.medium
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.workspaceSelected(pill.index)
                    }
                }
            }
        }
    }

    // --- launcher entry ---------------------------------------------------
    Rectangle {
        id: searchPill
        anchors.centerIn: parent
        width: Math.min(460, root.width * 0.38)
        height: 34
        radius: Radius.pill
        color: Theme.bgElevated
        border.width: 1
        border.color: searchArea.containsMouse ? Theme.borderStrong : Theme.border

        Row {
            anchors.left: parent.left
            anchors.leftMargin: Spacing.md
            anchors.verticalCenter: parent.verticalCenter
            spacing: Spacing.sm

            TagIcon { name: "search"; size: 15; dim: 0.8; anchors.verticalCenter: parent.verticalCenter }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Her şeyi ara…")
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.body
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: Spacing.sm
            anchors.verticalCenter: parent.verticalCenter
            width: shortcutText.implicitWidth + Spacing.md
            height: 20
            radius: Radius.small
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Text {
                id: shortcutText
                anchors.centerIn: parent
                text: "Ctrl + Space"
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
            }
        }

        MouseArea {
            id: searchArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.launcherRequested()
        }
    }

    // --- status + clock ---------------------------------------------------
    Row {
        id: rightCluster
        anchors.right: parent.right
        anchors.rightMargin: Spacing.xl
        anchors.verticalCenter: parent.verticalCenter
        spacing: Spacing.lg

        Repeater {
            model: [ "bell", "wifi", "volume", "battery" ]
            delegate: Item {
                required property string modelData
                width: 20
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                TagIcon { anchors.fill: parent; name: parent.modelData; size: 19; dim: 0.85 }
            }
        }

        Rectangle {
            width: 1
            height: 22
            color: Theme.border
            anchors.verticalCenter: parent.verticalCenter
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Text {
                id: clockText
                text: Qt.formatTime(new Date(), "hh:mm")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.medium
            }

            Text {
                id: dateText
                text: root.turkishDate(new Date())
                color: Theme.textDim
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
            }
        }
    }

    // Clicking the status area opens the control centre.
    MouseArea {
        anchors.right: parent.right
        anchors.rightMargin: Spacing.md
        anchors.verticalCenter: parent.verticalCenter
        width: rightCluster.width + Spacing.lg
        height: parent.height
        cursorShape: Qt.PointingHandCursor
        onClicked: root.controlCenterRequested()
    }

    Timer {
        interval: 10000
        running: true
        repeat: true
        onTriggered: {
            clockText.text = Qt.formatTime(new Date(), "hh:mm");
            dateText.text = root.turkishDate(new Date());
        }
    }

    function turkishDate(date) {
        const months = ["Oca", "Şub", "Mar", "Nis", "May", "Haz", "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara"];
        const days = ["Paz", "Pzt", "Sal", "Çar", "Per", "Cum", "Cmt"];
        return date.getDate() + " " + months[date.getMonth()] + ", " + days[date.getDay()];
    }
}
