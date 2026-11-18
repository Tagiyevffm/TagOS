import QtQuick

// In-memory notification centre for the shell.
//
// TagNotifyd (Phase 3) will push entries here over D-Bus; until then the shell
// itself files real events (session ready, app launch failures, power ask).
QtObject {
    id: root

    property int maxItems: 24
    readonly property int count: model.count

    property ListModel model: ListModel {}

    signal arrived(string title, string body)

    function notify(title, body, icon, urgency) {
        if (!title && !body)
            return;
        model.insert(0, {
            "title": title || "",
            "body": body || "",
            "icon": icon || "bell",
            "urgency": urgency || "normal",
            "seen": false,
            "stamp": new Date().toLocaleTimeString(Qt.locale("tr_TR"), "HH:mm")
        });
        while (model.count > root.maxItems)
            model.remove(model.count - 1);
        root.arrived(title, body);
        return true;
    }

    function dismiss(index) {
        if (index >= 0 && index < model.count)
            model.remove(index);
    }

    function clear() {
        model.clear();
    }

    function markAllSeen() {
        for (let i = 0; i < model.count; ++i)
            model.setProperty(i, "seen", true);
    }

    readonly property int unseen: {
        let n = 0;
        for (let i = 0; i < model.count; ++i) {
            if (!model.get(i).seen)
                ++n;
        }
        return n;
    }
}
