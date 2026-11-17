pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0

// Bottom dock: the running/quick-launch applications.
Item {
    id: root

    signal launcherRequested()
    signal appRequested(string exec)
    signal trashRequested()

    implicitWidth: surface.width
    implicitHeight: 56

    Rectangle {
        id: surface
        anchors.centerIn: parent
        width: dockRow.width + Spacing.xl * 2
        height: 56
        radius: Radius.large
        color: Theme.bgElevated
        border.width: 1
        border.color: Theme.border

        Row {
            id: dockRow
            anchors.centerIn: parent
            spacing: Spacing.md

            DockItem {
                icon: "grid"
                label: qsTr("Uygulamalar")
                highlighted: true
                onActivated: root.launcherRequested()
            }

            Rectangle {
                width: 1
                height: 26
                color: Theme.border
                anchors.verticalCenter: parent.verticalCenter
            }

            DockItem { icon: "folder"; label: "TagFiles"; onActivated: root.appRequested("tag-files") }
            DockItem { icon: "terminal"; label: "Terminal"; onActivated: root.appRequested("foot") }
            DockItem { icon: "monitor"; label: "TagMonitor"; onActivated: root.appRequested("tag-monitor") }
            DockItem { icon: "settings"; label: "Ayarlar"; onActivated: root.appRequested("tag-settings") }
            DockItem { icon: "store"; label: "TagStore"; onActivated: root.appRequested("tag-store") }

            Rectangle {
                width: 1
                height: 26
                color: Theme.border
                anchors.verticalCenter: parent.verticalCenter
            }

            DockItem { icon: "trash"; label: qsTr("Çöp"); onActivated: root.trashRequested() }
        }
    }

    component DockItem: Item {
        id: item

        property string icon: "grid"
        property string label: ""
        property bool highlighted: false

        signal activated()

        width: 40
        height: 40

        Rectangle {
            anchors.fill: parent
            radius: Radius.small
            color: item.highlighted ? Theme.accentSoft
                                    : (area.containsMouse ? Theme.surfaceHover : "transparent")
            border.width: item.highlighted ? 1 : 0
            border.color: Theme.accent

            Behavior on color { ColorAnimation { duration: Motion.fast } }
        }

        TagIcon {
            anchors.centerIn: parent
            name: item.icon
            size: 21
            dim: area.containsMouse || item.highlighted ? 1.0 : 0.9
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -8
            width: area.containsMouse || item.highlighted ? 4 : 0
            height: 3
            radius: 2
            color: Theme.accent
            Behavior on width { NumberAnimation { duration: Motion.fast } }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: item.activated()
        }
    }
}
