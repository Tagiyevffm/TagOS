import QtQuick
import QtQuick.Window
import TagOS.Design 1.0
import TagOS.Flow 1.0
import TagOS.System 1.0

// TagOS board — Phase 0 shell surface.
//
// Layer map (see docs/architecture.md):
//   wallpaper → board widgets → dock/panels → overlays (launcher, control centre)
// Everything binds to FlowClient for metrics; nothing here reads /proc.
Window {
    id: window

    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    color: Theme.bg
    title: "TagOS " + Theme.version
    flags: Qt.FramelessWindowHint

    // Two transports, one API: D-Bus to tag-flowd, or in-process on dev hosts.
    FlowClient {
        id: flow
        pollIntervalMs: 1500
    }

    AppCatalog { id: appCatalog }

    property bool controlCenterOpen: false

    Wallpaper { anchors.fill: parent }

    TopBar {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        hostName: flow.hostName
        onLauncherRequested: launcher.toggle()
        onControlCenterRequested: window.controlCenterOpen = !window.controlCenterOpen
        onWorkspaceSelected: (index) => window.controlCenterOpen = false

        // The shell reports the transport in the panel tooltip area; the card on
        // the right shows it in full.
    }

    SideDock {
        id: sideDock
        anchors.left: parent.left
        anchors.top: topBar.bottom
        anchors.bottom: bottomDock.top
        width: 84
        onStoreRequested: {
            launcher.show();
            launcher.query = "TagStore";
        }
        onPathRequested: (path) => AppLauncher.openPath(path)
    }

    Hero {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        onSearchRequested: launcher.show()
    }

    MonitorCard {
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.rightMargin: Spacing.xl
        anchors.topMargin: Spacing.xl
        metrics: flow
    }

    ControlCenter {
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.rightMargin: Spacing.xl
        anchors.topMargin: Spacing.xl
        metrics: flow
        visible: window.controlCenterOpen
        opacity: window.controlCenterOpen ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Motion.fast } }
    }

    BottomDock {
        id: bottomDock
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Spacing.md
        anchors.horizontalCenter: parent.horizontalCenter
        onLauncherRequested: launcher.toggle()
        onAppRequested: (exec) => AppLauncher.launch(exec)
        onTrashRequested: AppLauncher.openPath(".local/share/Trash")
    }

    LauncherOverlay {
        id: launcher
        anchors.fill: parent
        catalog: appCatalog
        onCloseRequested: hide()
    }

    // Global shortcuts (compositor-level bindings live in labwc rc.xml).
    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: launcher.toggle()
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: launcher.show()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "Escape"
        enabled: window.controlCenterOpen
        onActivated: window.controlCenterOpen = false
    }

    Component.onCompleted: {
        if (AppLauncher.dryRun)
            console.log("TagOS shell in dry-run mode (TAGOS_DRY_RUN=1)");
        console.log("TagOS shell ready · metrics transport: " + flow.transportLabel);
    }
}
