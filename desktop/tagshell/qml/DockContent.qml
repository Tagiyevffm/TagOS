import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0

// Floating dock: launcher, pinned applications, all-apps entry.
//
// Pinning writes to SettingsStore, so the dock, the desktop icons and TagSettings
// always agree on what is pinned.
Item {
    id: root

    property var apps: null
    property bool compact: false

    readonly property int iconSize: SettingsStore.dockIconSize
    readonly property int buttonSize: Math.round(iconSize * 1.35)

    signal requestLauncher()

    implicitHeight: buttonSize + Spacing.lg * 2
    implicitWidth: surface.width
    width: implicitWidth
    height: implicitHeight

    function pinnedIds() {
        const pinned = SettingsStore.pinnedApps;
        if (pinned.length > 0)
            return pinned;
        return ["tag-files.desktop", "firefox.desktop", "tag-monitor.desktop",
                "tag-settings.desktop", "tag-about.desktop"];
    }

    /// Bumped when the catalog reloads; referenced by dockApps() so the
    /// Repeater model re-evaluates (Q_INVOKABLE calls register no dependency).
    property int appsRevision: 0

    Connections {
        target: root.apps
        function onChanged() { root.appsRevision += 1; }
    }

    function dockApps() {
        const revision = root.appsRevision;   // dependency for the Repeater model
        if (!root.apps)
            return [];
        const result = [];
        for (const id of pinnedIds()) {
            const entry = root.apps.entry(id);
            if (entry && entry.id)
                result.push(entry);
        }
        return result;
    }

    function launch(id) {
        if (!root.apps || !id)
            return;
        if (root.apps.launch(id))
            SettingsStore.rememberLaunched(id);
    }

    Rectangle {
        id: surface
        anchors.verticalCenter: parent.verticalCenter
        width: row.implicitWidth + Spacing.lg * 2
        height: root.buttonSize + Spacing.sm * 2
        radius: Radius.large
        color: Theme.dark ? "#E6111318" : "#F7FFFFFF"
        border.width: 1
        border.color: Theme.border

        Row {
            id: row
            anchors.centerIn: parent
            spacing: Spacing.xs

            // Launcher
            Item {
                width: root.buttonSize
                height: root.buttonSize

                Rectangle {
                    anchors.fill: parent
                    radius: Radius.medium
                    color: launchHover.containsMouse ? Theme.accentSoft : "transparent"
                    border.width: 1
                    border.color: launchHover.containsMouse ? Theme.accent : "transparent"
                }

                TagIcon {
                    anchors.centerIn: parent
                    name: "tagos-logo"
                    size: Math.round(root.iconSize * 0.72)
                }

                MouseArea {
                    id: launchHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestLauncher()
                }
            }

            Rectangle {
                width: 1
                height: root.buttonSize * 0.6
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.border
            }

            Repeater {
                model: root.dockApps()

                delegate: Item {
                    id: appButton
                    required property var modelData
                    required property int index

                    width: root.buttonSize
                    height: root.buttonSize

                    Rectangle {
                        id: appSurface
                        anchors.centerIn: parent
                        width: root.buttonSize - 6
                        height: root.buttonSize - 6
                        radius: Radius.medium
                        color: appHover.containsMouse ? Theme.surfaceHover : Theme.surfaceAlt
                        border.width: 1
                        border.color: appHover.containsMouse ? Theme.accent : "transparent"
                        scale: appHover.containsMouse ? 1.06 : 1.0

                        Behavior on scale { NumberAnimation { duration: Motion.fast; easing.type: Motion.easing } }
                        Behavior on color { ColorAnimation { duration: Motion.fast } }
                    }

                    TagIcon {
                        anchors.centerIn: appSurface
                        name: appButton.modelData.icon && appButton.modelData.icon.length > 0
                              ? appButton.modelData.icon : "app"
                        size: Math.round(root.iconSize * 0.72)
                    }

                    Rectangle {
                        visible: SettingsStore.isPinned(appButton.modelData.id)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.accent
                    }

                    MouseArea {
                        id: appHover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton) {
                                menuTarget = appButton.modelData;
                                appMenu.popup(appButton, appButton.width / 2 - 96,
                                              -appMenu.implicitHeight - 10);
                            } else {
                                root.launch(appButton.modelData.id);
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: 1
                height: root.buttonSize * 0.6
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.border
            }

            Item {
                width: root.buttonSize
                height: root.buttonSize

                Rectangle {
                    anchors.fill: parent
                    radius: Radius.medium
                    color: allHover.containsMouse ? Theme.surfaceHover : "transparent"
                }

                TagIcon {
                    anchors.centerIn: parent
                    name: "grid"
                    size: Math.round(root.iconSize * 0.66)
                    opacity: 0.85
                }

                MouseArea {
                    id: allHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestLauncher()
                }
            }
        }
    }

    property var menuTarget: null

    TagMenu {
        id: appMenu
        menuWidth: 192
        items: root.menuTarget === null ? [] : [
            { label: qsTr("Aç"), icon: "app" },
            { separator: true },
            { label: SettingsStore.isPinned(root.menuTarget.id) ? qsTr("Dock'tan kaldır") : qsTr("Dock'a sabitle"),
              icon: SettingsStore.isPinned(root.menuTarget.id) ? "trash" : "plus" }
        ]
        onTriggered: index => {
            if (root.menuTarget === null)
                return;
            if (index === 0) {
                root.launch(root.menuTarget.id);
            } else if (index === 2) {
                if (SettingsStore.isPinned(root.menuTarget.id))
                    SettingsStore.unpinApp(root.menuTarget.id);
                else
                    SettingsStore.pinApp(root.menuTarget.id);
            }
        }
    }
}
