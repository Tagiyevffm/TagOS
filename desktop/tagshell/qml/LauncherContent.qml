import QtQuick
import TagOS.Design 1.0
import TagOS.Config 1.0

// TagLaunch: global search over applications, recent applications, and command
// execution (`>` prefix). Keyboard first: type, arrow, Enter, Escape.
Item {
    id: root

    property var apps: null
    property var notifications: null
    property string query: ""
    property int selectedIndex: 0
    property var results: []

    signal dismissed()
    signal launched(string id)

    function appResult(entry, recent) {
        return {
            "kind": "app",
            "id": entry.id,
            "title": entry.name,
            "subtitle": recent ? qsTr("Son kullanılan") : (entry.genericName || entry.comment || entry.id),
            "icon": entry.icon && entry.icon.length > 0 ? entry.icon : "app"
        };
    }

    function buildResults(text) {
        const out = [];
        if (!root.apps)
            return out;

        const query = text.trim();

        if (query.startsWith(">")) {
            const command = query.slice(1).trim();
            if (command.length > 0) {
                out.push({
                    "kind": "command",
                    "id": "$command",
                    "title": qsTr("Komut çalıştır"),
                    "subtitle": command,
                    "icon": "terminal",
                    "command": command
                });
            }
            return out;
        }

        if (query.length === 0) {
            const seen = {};
            for (const id of SettingsStore.recentApps) {
                const entry = root.apps.entry(id);
                if (entry && entry.id && !seen[entry.id]) {
                    seen[entry.id] = true;
                    out.push(root.appResult(entry, true));
                }
                if (out.length >= 4)
                    break;
            }
            for (const entry of root.apps.search("", 12)) {
                if (seen[entry.id])
                    continue;
                seen[entry.id] = true;
                out.push(root.appResult(entry, false));
                if (out.length >= 10)
                    break;
            }
            return out;
        }

        for (const entry of root.apps.search(query, 10))
            out.push(root.appResult(entry, false));
        return out;
    }

    function refresh() {
        results = buildResults(query);
        selectedIndex = results.length > 0 ? 0 : -1;
    }

    function activate(index) {
        if (index < 0 || index >= results.length)
            return;
        const item = results[index];
        if (item.kind === "command") {
            if (root.apps.launchCommand(item.command))
                root.launched(item.command);
            return;
        }
        if (root.apps.launch(item.id)) {
            SettingsStore.rememberLaunched(item.id);
            root.launched(item.id);
        } else if (root.notifications) {
            root.notifications.notify(qsTr("Başlatılamadı"), item.title, "warning");
        }
    }

    onQueryChanged: refresh()
    onAppsChanged: refresh()
    Component.onCompleted: refresh()

    Connections {
        target: root.apps
        function onChanged() { root.refresh(); }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.overlay
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.dismissed()
    }

    Rectangle {
        id: panel
        width: Math.min(700, root.width - Spacing.xxxl * 2)
        height: Math.min(520, searchColumn.implicitHeight + Spacing.lg * 2)
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(root.height * 0.14)
        radius: Radius.large
        color: Theme.card
        border.width: 1
        border.color: Theme.borderStrong

        MouseArea { anchors.fill: parent }   // swallow clicks behind the panel

        Column {
            id: searchColumn
            anchors.fill: parent
            anchors.margins: Spacing.lg
            spacing: Spacing.md

            Row {
                width: parent.width
                spacing: Spacing.md

                TagIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "search"
                    size: 20
                    opacity: 0.8
                }

                TextInput {
                    id: input
                    width: parent.width - 60
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.text
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.accentInk
                    font.family: Typography.family
                    font.pixelSize: Typography.title
                    focus: true
                    clip: true
                    onTextChanged: root.query = text

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: input.text.length === 0
                        text: qsTr("Uygulama, dosya ya da komut ara…")
                        color: Theme.textMuted
                        font: input.font
                    }

                    Keys.onDownPressed: event => {
                        if (root.results.length === 0)
                            return;
                        root.selectedIndex = Math.min(root.results.length - 1, root.selectedIndex + 1);
                        event.accepted = true;
                    }
                    Keys.onUpPressed: event => {
                        if (root.results.length === 0)
                            return;
                        root.selectedIndex = Math.max(0, root.selectedIndex - 1);
                        event.accepted = true;
                    }
                    Keys.onReturnPressed: event => {
                        root.activate(root.selectedIndex);
                        event.accepted = true;
                    }
                    Keys.onEnterPressed: event => {
                        root.activate(root.selectedIndex);
                        event.accepted = true;
                    }
                    Keys.onEscapePressed: event => {
                        root.dismissed();
                        event.accepted = true;
                    }
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44
                    height: 24
                    radius: Radius.small
                    color: Theme.surfaceAlt
                    border.width: 1
                    border.color: Theme.border

                    Text {
                        anchors.centerIn: parent
                        text: "Esc"
                        color: Theme.textMuted
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption - 1
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border }

            ListView {
                id: list
                width: parent.width
                height: Math.min(360, contentHeight)
                clip: true
                model: root.results
                currentIndex: root.selectedIndex
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds

                delegate: TagListRow {
                    required property var modelData
                    required property int index

                    width: list.width
                    icon: modelData.icon
                    title: modelData.title
                    subtitle: modelData.subtitle
                    selected: index === root.selectedIndex
                    trailing: modelData.kind === "command" ? qsTr("Komut") : ""
                    onClicked: {
                        root.selectedIndex = index;
                        root.activate(index);
                    }
                }

                onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            }

            Item {
                width: parent.width
                height: root.results.length === 0 ? 96 : 0

                TagEmptyState {
                    anchors.centerIn: parent
                    visible: root.results.length === 0
                    icon: "search-file"
                    title: qsTr("Sonuç yok")
                    message: qsTr("Farklı bir arama deneyin. Komut çalıştırmak için başına “>” yazın.")
                }
            }

            Row {
                width: parent.width
                spacing: Spacing.lg

                Text {
                    text: qsTr("↑↓ gez")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
                Text {
                    text: qsTr("Enter aç")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
                Text {
                    text: qsTr("> komut")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }

                Item { width: parent.width - 320; height: 1 }

                Text {
                    text: root.results.length > 0 ? qsTr("%1 sonuç").arg(root.results.length) : ""
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }
        }
    }

    function focusInput() {
        input.forceActiveFocus();
        input.selectAll();
    }
}
