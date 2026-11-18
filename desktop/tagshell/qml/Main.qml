pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0
import TagOS.Flow 1.0
import TagOS.Apps 1.0
import TagOS.Control 1.0
import TagOS.System 1.0

// TagOS shell — board surface.
//
// One frameless full-screen window hosts the merged desktop composition
// (wallpaper, panel, dock, overlays). Panels are separate reusable Items, so the
// same components can be promoted to wlr-layer-shell surfaces without touching
// their content: see docs/architecture.md → "Surface model".
Window {
    id: board

    objectName: "board"
    title: "TagOS"

    /// Set by tag-shell from TAGOS_SHELL_STATE (dev screenshots / UI tests).
    property string startupState: ""
    onStartupStateChanged: {
        if (startupState === "launcher")
            openLauncher();
        else if (startupState === "center")
            toggleCenter();
        else if (startupState === "dock")
            notifications.notify(qsTr("Dosyalar kopyalandı"), qsTr("12 öğe · /home/kullanıcı/Belgeler"), "file");
    }

    color: Theme.bg
    flags: Qt.FramelessWindowHint
    visible: true
    width: 1280
    height: 800
    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    // --- shared models ----------------------------------------------------
    FlowClient {
        id: flow
        pollIntervalMs: 2000
    }

    // AppCatalog and SystemControl are QML singletons: one scan, one poll,
    // shared by the shell and every TagOS app.
    readonly property var apps: AppCatalog
    readonly property var control: SystemControl
    NotificationCenter { id: notifications }

    // System state is polled slowly; TagCenter refreshes on open as well.
    Timer {
        interval: 15000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: control.refresh()
    }

    // --- desktop ----------------------------------------------------------
    BoardContent {
        id: desktop
        anchors.fill: parent
        flow: flow
        apps: board.apps
        onRequestLauncher: board.openLauncher()
        onRequestSettings: board.launchApp("tag-settings.desktop", qsTr("TagSettings"))
        onRequestMonitor: board.launchApp("tag-monitor.desktop", qsTr("TagMonitor"))
        onRequestAbout: board.launchApp("tag-about.desktop", qsTr("TagOS Hakkında"))
    }

    // --- chrome -----------------------------------------------------------
    Item {
        id: chrome
        anchors.fill: parent

        TopPanelContent {
            id: topPanel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            flow: flow
            apps: board.apps
            control: board.control
            notifications: board.notifications
            onRequestLauncher: board.openLauncher()
            onRequestCenter: board.toggleCenter()
            onRequestPower: board.openPowerMenu()
        }

        Item {
            id: dockHost
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: dock.height + Spacing.lg

            property bool revealed: !SettingsStore.dockAutoHide || dockMouse.containsMouse

            MouseArea {
                id: dockMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                onContainsMouseChanged: {
                    if (!SettingsStore.dockAutoHide)
                        dockHost.revealed = true;
                }
            }

            DockContent {
                id: dock
                anchors.horizontalCenter: parent.horizontalCenter
                y: dockHost.revealed ? Spacing.sm : dock.height + Spacing.md
                apps: board.apps
                onRequestLauncher: board.openLauncher()

                Behavior on y {
                    NumberAnimation { duration: Motion.normal; easing.type: Motion.easing }
                }
            }
        }

        NotificationStack {
            id: toasts
            anchors.right: parent.right
            anchors.rightMargin: Spacing.xl
            anchors.bottom: dockHost.top
            anchors.bottomMargin: Spacing.md
            notifications: board.notifications
            visible: !center.opened && notifications.count > 0
        }

        ControlCenterContent {
            id: center
            z: 10
            anchors.right: parent.right
            anchors.rightMargin: Spacing.md
            anchors.top: parent.top
            anchors.topMargin: 52
            control: board.control
            notifications: board.notifications
            apps: board.apps

            property bool opened: opacity > 0.01

            opacity: 0
            visible: opacity > 0.01
            onRequestDismiss: board.closeCenter()
            onRequestSettings: board.launchApp("tag-settings.desktop", qsTr("TagSettings"))

            Behavior on opacity { NumberAnimation { duration: Motion.fast } }
        }

        LauncherContent {
            id: launcher
            z: 20
            anchors.fill: parent
            apps: board.apps
            notifications: board.notifications
            opacity: 0
            visible: opacity > 0.01
            onDismissed: board.closeLauncher()
            onLaunched: board.closeLauncher()

            Behavior on opacity { NumberAnimation { duration: Motion.fast } }
        }

        // Clicking the board closes the control centre (launcher has its own
        // scrim covering everything).
        MouseArea {
            z: 9
            anchors.fill: parent
            enabled: center.opened
            visible: enabled
            onClicked: board.closeCenter()
        }
    }

    // --- shell actions ----------------------------------------------------
    function openLauncher() {
        closeCenter();
        launcher.opacity = 1;
        launcher.focusInput();
    }

    function closeLauncher() {
        launcher.opacity = 0;
    }

    function toggleCenter() {
        if (center.opened) {
            closeCenter();
        } else {
            launcher.opacity = 0;
            center.opacity = 1;
            notifications.markAllSeen();
            control.refresh();
        }
    }

    function closeCenter() {
        center.opacity = 0;
    }

    function launchApp(id, label) {
        if (!apps.launch(id)) {
            notifications.notify(label || id,
                                 qsTr("Uygulama başlatılamadı: %1").arg(apps.lastError),
                                 "warning",
                                 "high");
            return false;
        }
        SettingsStore.rememberLaunched(id);
        return true;
    }

    function openPowerMenu() {
        powerMenu.popup();
    }

    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: launcher.opacity > 0.01 ? board.closeLauncher() : board.openLauncher()
    }

    Shortcut {
        sequence: "Ctrl+Alt+T"
        onActivated: board.launchApp("tag-terminal.desktop", qsTr("TagTerminal"))
    }

    Shortcut {
        sequence: "Escape"
        enabled: launcher.opacity > 0.01 || center.opened
        onActivated: {
            board.closeLauncher();
            board.closeCenter();
        }
    }

    TagMenu {
        id: powerMenu
        items: [
            { label: qsTr("Yeniden başlat"), icon: "refresh" },
            { label: qsTr("Kapat"), icon: "power", danger: true },
            { label: qsTr("Oturumu kapat"), icon: "user" },
            { separator: true },
            { label: qsTr("Ayarlar"), icon: "settings" }
        ]
        onTriggered: index => {
            if (index === 0)
                PowerManager.reboot();
            else if (index === 1)
                PowerManager.powerOff();
            else if (index === 2)
                PowerManager.logOut();
            else if (index === 4)
                board.launchApp("tag-settings.desktop", qsTr("TagSettings"));
        }
    }

    Timer {
        interval: 900
        running: true
        onTriggered: notifications.notify(qsTr("TagOS hazır"),
                                          qsTr("Kabuk %1 · %2")
                                              .arg(Theme.version)
                                              .arg(flow.valid ? flow.transportLabel : qsTr("yerel ölçüm")),
                                          "tagos-logo")
    }

    Component.onCompleted: {
        if (SettingsStore.lastError.length > 0)
            notifications.notify(qsTr("Ayarlar yüklenemedi"), SettingsStore.lastError, "warning", "high");
    }
}
