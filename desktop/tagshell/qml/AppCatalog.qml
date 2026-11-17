import QtQuick

// TagOS application catalog.
//
// Phase 0 ships the shell and TagMonitor only, so entries carry an explicit
// availability flag and their roadmap phase. The launcher shows planned apps
// dimmed instead of pretending they exist.
QtObject {
    id: catalog

    readonly property var apps: [
        { name: "TagFiles", icon: "folder", exec: "tag-files", category: "Dosyalar", available: false, phase: 3 },
        { name: "TagSettings", icon: "settings", exec: "tag-settings", category: "Sistem", available: false, phase: 3 },
        { name: "TagMonitor", icon: "monitor", exec: "tag-monitor", category: "Sistem", available: true, phase: 3 },
        { name: "TagStore", icon: "store", exec: "tag-store", category: "Yazılım", available: false, phase: 4 },
        { name: "TagUpdate", icon: "refresh", exec: "tag-update", category: "Sistem", available: false, phase: 4 },
        { name: "TagSnapshot", icon: "layers", exec: "tag-snapshot", category: "Sistem", available: false, phase: 4 },
        { name: "TagAI", icon: "cpu", exec: "tag-ai", category: "Yapay zekâ", available: false, phase: 4 },
        { name: "Terminal", icon: "terminal", exec: "foot", category: "Araçlar", available: true, phase: 0 },
        { name: "Dosyalar", icon: "folder", exec: "thunar", category: "Araçlar", available: false, phase: 3 },
        { name: "Tarayıcı", icon: "grid", exec: "firefox", category: "İnternet", available: false, phase: 4 },
        { name: "Hakkında", icon: "shield", exec: "tag-about", category: "Sistem", available: false, phase: 3 }
    ]

    function filtered(query) {
        const text = (query || "").trim().toLowerCase();
        if (text.length === 0)
            return apps;

        const result = [];
        for (let i = 0; i < apps.length; ++i) {
            const app = apps[i];
            if (app.name.toLowerCase().indexOf(text) !== -1
                    || app.category.toLowerCase().indexOf(text) !== -1
                    || app.exec.toLowerCase().indexOf(text) !== -1) {
                result.push(app);
            }
        }
        return result;
    }

    function launchable(app) {
        return app !== undefined && app !== null && app.available;
    }
}
