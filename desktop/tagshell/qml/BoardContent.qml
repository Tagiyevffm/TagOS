import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0

// The desktop layer: wallpaper, launcher icons, clock widget and the system
// pulse card. Lives inside the shell's board window (composited mode).
Item {
    id: root

    property var flow: null
    property var apps: null
    property bool showIcons: true

    signal requestLauncher()
    signal requestSettings()
    signal requestMonitor()
    signal requestAbout()

    property int appsRevision: 0

    Connections {
        target: root.apps
        function onChanged() { root.appsRevision += 1; }
    }

    function defaultApps() {
        if (!root.apps)
            return [];
        const preferred = ["tag-files.desktop", "tag-monitor.desktop",
                           "tag-settings.desktop", "tag-about.desktop"];
        const result = [];
        for (const id of preferred) {
            const found = root.apps.entry(id);
            if (found && found.id)
                result.push(found);
        }
        return result;
    }

    function desktopApps() {
        const revision = root.appsRevision;   // dependency for the Repeater model
        if (!root.apps)
            return [];
        const pinned = SettingsStore.pinnedApps;
        const source = pinned.length > 0 ? pinned : null;
        if (!source)
            return defaultApps();

        const result = [];
        for (const id of source) {
            const found = root.apps.entry(id);
            if (found && found.id)
                result.push(found);
        }
        return result.length > 0 ? result : defaultApps();
    }

    function launch(id) {
        if (!root.apps || !id)
            return;
        if (root.apps.launch(id))
            SettingsStore.rememberLaunched(id);
    }

    Wallpaper { anchors.fill: parent }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: desktopMenu.popup()
    }

    // --- launcher icons ---------------------------------------------------
    Column {
        id: iconColumn
        visible: root.showIcons
        anchors.left: parent.left
        anchors.leftMargin: Spacing.xl
        anchors.top: parent.top
        anchors.topMargin: Spacing.xxxl + Spacing.xl
        spacing: Spacing.md

        Repeater {
            model: root.desktopApps()

            delegate: Item {
                id: tile
                required property var modelData

                width: 84
                height: 78

                Rectangle {
                    id: tileSurface
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 54
                    height: 54
                    radius: Radius.medium
                    color: hover.containsMouse ? Theme.surfaceHover : Theme.surfaceAlt
                    border.width: 1
                    border.color: hover.containsMouse ? Theme.accent : Theme.border

                    Behavior on color { ColorAnimation { duration: Motion.fast } }
                }

                TagIcon {
                    anchors.centerIn: tileSurface
                    name: tile.modelData.icon && tile.modelData.icon.length > 0 ? tile.modelData.icon : "app"
                    size: 26
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: tileSurface.bottom
                    anchors.topMargin: Spacing.xs
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: tile.modelData.name
                    color: hover.containsMouse ? Theme.text : Theme.textDim
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.launch(tile.modelData.id)
                }
            }
        }
    }

    Hero {
        id: hero
        flow: root.flow
        anchors.left: parent.left
        anchors.leftMargin: Spacing.xxxl
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 108
    }

    MonitorCard {
        anchors.right: parent.right
        anchors.rightMargin: Spacing.xl
        anchors.top: parent.top
        anchors.topMargin: Spacing.xxxl + Spacing.md
        metrics: root.flow
    }

    TagMenu {
        id: desktopMenu
        items: [
            { label: qsTr("Uygulamaları aç"), icon: "grid", shortcut: "Ctrl+Space" },
            { label: qsTr("Sistem monitörü"), icon: "monitor" },
            { label: qsTr("Ayarlar"), icon: "settings" },
            { separator: true },
            { label: qsTr("Masaüstünü yenile"), icon: "refresh" },
            { label: qsTr("TagOS hakkında"), icon: "info" }
        ]
        onTriggered: index => {
            if (index === 0)
                root.requestLauncher();
            else if (index === 1)
                root.requestMonitor();
            else if (index === 2)
                root.requestSettings();
            else if (index === 4 && root.apps)
                root.apps.refresh();
            else if (index === 5)
                root.requestAbout();
        }
    }
}
