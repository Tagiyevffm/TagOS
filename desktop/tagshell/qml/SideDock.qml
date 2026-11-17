pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0

// Vertical quick-access rail on the left edge of the board.
Item {
    id: root

    signal launcherRequested()
    signal pathRequested(string path)
    signal storeRequested()

    implicitWidth: 84

    Column {
        anchors.top: parent.top
        anchors.topMargin: Spacing.xxl
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Spacing.lg

        Repeater {
            model: [
                { icon: "home", label: qsTr("Ev"), action: "home" },
                { icon: "trash", label: qsTr("Çöp"), action: "trash" },
                { icon: "folder", label: qsTr("Projeler"), action: "projects" },
                { icon: "store", label: qsTr("TagStore"), action: "store" }
            ]

            delegate: Item {
                id: entry
                required property var modelData
                width: 68
                height: 62

                Rectangle {
                    anchors.fill: parent
                    radius: Radius.medium
                    color: area.containsMouse ? Theme.surfaceAlt : "transparent"
                    Behavior on color { ColorAnimation { duration: Motion.fast } }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: Spacing.xs

                    TagIcon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        name: entry.modelData.icon
                        size: 22
                        dim: area.containsMouse ? 1.0 : 0.85
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: entry.modelData.label
                        color: area.containsMouse ? Theme.text : Theme.textDim
                        font.family: Typography.family
                        font.pixelSize: Typography.caption - 1
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        switch (entry.modelData.action) {
                        case "home":
                            root.pathRequested("");
                            break;
                        case "trash":
                            root.pathRequested(".local/share/Trash");
                            break;
                        case "projects":
                            root.pathRequested("Projects");
                            break;
                        case "store":
                            root.storeRequested();
                            break;
                        }
                    }
                }
            }
        }
    }
}
