pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import TagOS.Apps 1.0
import TagOS.Design 1.0
import TagOS.Files 1.0
import TagOS.Flow 1.0

// TagFiles — TagOS file manager.
//
// Real filesystem operations through TagOS.Files: browse, search, create, copy,
// move, rename, trash and permanent delete (with confirmation). Properties and
// previews come from the same module, so nothing here guesses about the disk.
TagAppWindow {
    id: root

    iconName: "tag-files"
    titleText: qsTr("TagFiles")
    subtitle: dir.path.length > 0 ? PathUtils.display(dir.path) : ""
    width: 1120
    height: 720

    property string viewMode: "grid"
    property string searchText: ""
    property var selectedPaths: []
    property bool propertiesOpen: true
    property string pendingRename: ""
    property string renamingText: ""
    property string pendingDelete: ""
    property var history: []
    property int historyIndex: -1

    readonly property string currentPath: dir.path
    readonly property int selectionCount: selectedPaths.length
    readonly property bool hasSelection: selectionCount > 0
    readonly property var preview: root.hasSelection ? Preview.describe(root.selectedPaths[0]) : ({})

    DirModel { id: dir }
    FlowClient { id: flow }

    Component.onCompleted: navigate(PathUtils.homePath(), true)

    // --- navigation -------------------------------------------------------
    function navigate(path, resetHistory) {
        if (!dir.setPath(path))
            return;
        if (resetHistory) {
            history = [path];
            historyIndex = 0;
        } else if (history[historyIndex] !== path) {
            const trimmed = history.slice(0, historyIndex + 1);
            trimmed.push(path);
            history = trimmed;
            historyIndex = trimmed.length - 1;
        }
        selectedPaths = [];
        searchText = "";
    }

    function goBack() {
        if (historyIndex > 0) {
            historyIndex -= 1;
            dir.setPath(history[historyIndex]);
            selectedPaths = [];
        }
    }

    function goForward() {
        if (historyIndex < history.length - 1) {
            historyIndex += 1;
            dir.setPath(history[historyIndex]);
            selectedPaths = [];
        }
    }

    function goUp() {
        const parent = PathUtils.parentOf(dir.path);
        if (parent.length > 0)
            navigate(parent, false);
    }

    function openPath(path, isDir) {
        if (isDir) {
            navigate(path, false);
            return;
        }
        if (!AppCatalog.launchFile(path))
            showMessage(qsTr("Dosya açılamadı"), AppCatalog.lastError);
    }

    function select(path, additive) {
        if (additive) {
            const next = selectedPaths.slice();
            const at = next.indexOf(path);
            if (at >= 0)
                next.splice(at, 1);
            else
                next.push(path);
            selectedPaths = next;
        } else {
            selectedPaths = [path];
        }
    }

    function refreshAfter(okFlag) {
        if (!okFlag)
            showMessage(qsTr("İşlem başarısız"), FileOps.lastError());
        dir.refresh();
        selectedPaths = [];
    }

    signal message(string title, string body)

    function showMessage(title, body) {
        message(title, body);
    }

    // --- actions ----------------------------------------------------------
    function copySelection() {
        if (!hasSelection)
            return;
        clipboardPaths = selectedPaths.slice();
        clipboardCut = false;
    }

    function cutSelection() {
        if (!hasSelection)
            return;
        clipboardPaths = selectedPaths.slice();
        clipboardCut = true;
    }

    function paste() {
        if (clipboardPaths.length === 0)
            return;
        const ok = clipboardCut ? FileOps.move(clipboardPaths, dir.path)
                                : FileOps.copy(clipboardPaths, dir.path);
        if (ok) {
            if (clipboardCut)
                clipboardPaths = [];
            dir.refresh();
            selectedPaths = [];
        } else {
            showMessage(qsTr("Yapıştırılamadı"), FileOps.lastError());
        }
    }

    function trashSelection() {
        if (!hasSelection)
            return;
        const targets = selectedPaths.slice();
        if (!FileOps.moveToTrash(targets)) {
            // No freedesktop trash available: require the explicit permanent path.
            pendingDelete = targets.join("\n");
            return;
        }
        dir.refresh();
        selectedPaths = [];
    }

    function deletePermanently() {
        if (pendingDelete.length === 0)
            return;
        const targets = pendingDelete.split("\n");
        const ok = FileOps.removePermanently(targets);
        pendingDelete = "";
        if (!ok)
            showMessage(qsTr("Silinemedi"), FileOps.lastError());
        dir.refresh();
        selectedPaths = [];
    }

    function startRename() {
        if (selectionCount !== 1)
            return;
        pendingRename = selectedPaths[0];
        renamingText = FileOps.baseName(pendingRename);
        renameDialog.open();
    }

    function applyRename() {
        if (pendingRename.length === 0)
            return;
        const ok = FileOps.rename(pendingRename, renamingText);
        pendingRename = "";
        if (!ok)
            showMessage(qsTr("Yeniden adlandırılamadı"), FileOps.lastError());
        dir.refresh();
        selectedPaths = [];
    }

    function newFolder() {
        const name = qsTr("Yeni Klasör");
        let candidate = name;
        let counter = 2;
        while (FileOps.copyNameFor(dir.path, candidate) !== candidate)
            candidate = name + " " + counter++;
        if (!FileOps.makeDir(dir.path, candidate))
            showMessage(qsTr("Oluşturulamadı"), FileOps.lastError());
        dir.refresh();
    }

    // --- state ------------------------------------------------------------
    property var clipboardPaths: []
    property bool clipboardCut: false

    // --- layout -----------------------------------------------------------
    Row {
        anchors.fill: parent

        // Sidebar
        Rectangle {
            width: 212
            height: parent.height
            color: Theme.surface

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.border
            }

            Column {
                anchors.fill: parent
                anchors.margins: Spacing.sm
                spacing: 2

                Text {
                    x: Spacing.sm
                    y: Spacing.xs
                    text: qsTr("Yerler")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }

                Item { width: 1; height: Spacing.sm }

                Repeater {
                    model: [
                        { label: qsTr("Ev"), icon: "home", path: PathUtils.homePath() },
                        { label: qsTr("Masaüstü"), icon: "grid", path: PathUtils.join(PathUtils.homePath(), "Desktop") },
                        { label: qsTr("Belgeler"), icon: "file", path: PathUtils.join(PathUtils.homePath(), "Documents") },
                        { label: qsTr("İndirilenler"), icon: "arrow-down", path: PathUtils.join(PathUtils.homePath(), "Downloads") },
                        { label: qsTr("Resimler"), icon: "image", path: PathUtils.join(PathUtils.homePath(), "Pictures") },
                        { label: qsTr("Müzik"), icon: "volume", path: PathUtils.join(PathUtils.homePath(), "Music") },
                        { label: qsTr("Videolar"), icon: "play", path: PathUtils.join(PathUtils.homePath(), "Videos") },
                        { label: qsTr("Kök"), icon: "disk", path: "/" }
                    ]

                    delegate: Item {
                        required property var modelData

                        width: 196
                        height: 34

                        Rectangle {
                            anchors.fill: parent
                            radius: Radius.small
                            color: dir.path === modelData.path
                                   ? Theme.accentSoft
                                   : (placeHover.containsMouse ? Theme.surfaceHover : "transparent")
                            border.width: dir.path === modelData.path ? 1 : 0
                            border.color: Theme.accent
                        }

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: Spacing.sm
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Spacing.sm

                            TagIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: modelData.icon
                                size: 15
                                opacity: 0.8
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: Theme.textDim
                                font.family: Typography.family
                                font.pixelSize: Typography.body
                            }
                        }

                        MouseArea {
                            id: placeHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: navigate(modelData.path, false)
                        }
                    }
                }

                Item { width: 1; height: 1 }

                Rectangle { width: 196; height: 1; color: Theme.border }

                Item { width: 1; height: Spacing.sm }

                Text {
                    x: Spacing.sm
                    text: qsTr("Disk")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }

                TagRingGauge {
                    x: Spacing.sm
                    width: 64
                    height: 64
                    thickness: 6
                    valueFontSize: Typography.small
                    value: flow.diskUsedPercent
                    progressColor: Theme.info
                }

                Text {
                    x: Spacing.sm
                    width: 180
                    text: flow.diskText
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption - 1
                    wrapMode: Text.Wrap
                }
            }
        }

        // Content
        Item {
            width: parent.width - 212
            height: parent.height

            Column {
                anchors.fill: parent

                // Toolbar
                Rectangle {
                    width: parent.width
                    height: 46
                    color: Theme.bg

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: Spacing.sm
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Spacing.xxs

                        TagIconButton {
                            icon: "chevron-right"
                            rotation: 180
                            tooltip: qsTr("Geri")
                            enabled: root.historyIndex > 0
                            opacity: enabled ? 1 : 0.35
                            onClicked: root.goBack()
                        }

                        TagIconButton {
                            icon: "chevron-right"
                            tooltip: qsTr("İleri")
                            enabled: root.historyIndex < root.history.length - 1
                            opacity: enabled ? 1 : 0.35
                            onClicked: root.goForward()
                        }

                        TagIconButton {
                            icon: "arrow-up"
                            tooltip: qsTr("Üst klasör")
                            onClicked: root.goUp()
                        }

                        TagIconButton {
                            icon: "refresh"
                            tooltip: qsTr("Yenile")
                            onClicked: dir.refresh()
                        }

                        Rectangle { width: 1; height: 20; color: Theme.border }

                        TagIconButton {
                            icon: "folder-plus"
                            tooltip: qsTr("Yeni klasör")
                            onClicked: root.newFolder()
                        }
                    }

                    // Breadcrumb
                    Row {
                        id: breadcrumb
                        anchors.left: parent.left
                        anchors.leftMargin: 190
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Spacing.xs

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: PathUtils.display(dir.path)
                            color: Theme.text
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                            elide: Text.ElideMiddle
                            width: Math.min(implicitWidth, root.width * 0.32)
                        }
                    }

                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: Spacing.sm
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Spacing.xs

                        Rectangle {
                            width: 190
                            height: 28
                            radius: Radius.small
                            color: Theme.surfaceAlt
                            border.width: 1
                            border.color: searchField.activeFocus ? Theme.accent : Theme.border

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: Spacing.sm
                                anchors.rightMargin: Spacing.sm
                                spacing: Spacing.xs

                                TagIcon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    name: "search"
                                    size: 13
                                    opacity: 0.7
                                }

                                TextInput {
                                    id: searchField
                                    width: parent.width - 24
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.text
                                    font.family: Typography.family
                                    font.pixelSize: Typography.caption
                                    clip: true
                                    onTextChanged: root.searchText = text

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: searchField.text.length === 0
                                        text: qsTr("Klasörde ara…")
                                        color: Theme.textMuted
                                        font: searchField.font
                                    }
                                }
                            }
                        }

                        TagIconButton {
                            icon: "list"
                            tooltip: qsTr("Liste görünümü")
                            checked: root.viewMode === "list"
                            onClicked: root.viewMode = "list"
                        }

                        TagIconButton {
                            icon: "grid-view"
                            tooltip: qsTr("Izgara görünümü")
                            checked: root.viewMode === "grid"
                            onClicked: root.viewMode = "grid"
                        }

                        TagIconButton {
                            icon: "shield"
                            tooltip: qsTr("Gizli dosyalar")
                            checked: dir.showHidden
                            onClicked: dir.showHidden = !dir.showHidden
                        }

                        TagIconButton {
                            icon: "info"
                            tooltip: qsTr("Özellikler")
                            checked: root.propertiesOpen
                            onClicked: root.propertiesOpen = !root.propertiesOpen
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Theme.border }

                // Listing + properties
                Row {
                    width: parent.width
                    height: parent.height - 46 - 29 - 1

                    Item {
                        id: listingArea
                        width: root.propertiesOpen ? parent.width - 272 : parent.width
                        height: parent.height

                        GridView {
                            id: gridView
                            anchors.fill: parent
                            anchors.margins: Spacing.md
                            visible: root.viewMode === "grid" && !root.searchActive()
                            clip: true
                            model: dir
                            cellWidth: 108
                            cellHeight: 104
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: Item {
                                id: gridCell
                                required property string name
                                required property string filePath
                                required property bool isDir
                                required property string iconName

                                width: gridView.cellWidth
                                height: gridView.cellHeight

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    radius: Radius.small
                                    color: root.selectedPaths.indexOf(gridCell.filePath) >= 0
                                           ? Theme.accentSoft
                                           : (cellHover.containsMouse ? Theme.surfaceHover : "transparent")
                                    border.width: root.selectedPaths.indexOf(gridCell.filePath) >= 0 ? 1 : 0
                                    border.color: Theme.accent
                                }

                                Column {
                                    anchors.centerIn: parent
                                    spacing: Spacing.xs

                                    TagIcon {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        name: gridCell.iconName
                                        size: 32
                                    }

                                    Text {
                                        width: gridView.cellWidth - 12
                                        horizontalAlignment: Text.AlignHCenter
                                        text: gridCell.name
                                        color: Theme.text
                                        font.family: Typography.family
                                        font.pixelSize: Typography.caption
                                        elide: Text.ElideMiddle
                                        maximumLineCount: 2
                                        wrapMode: Text.Wrap
                                    }
                                }

                                MouseArea {
                                    id: cellHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: mouse => {
                                        root.select(gridCell.filePath,
                                                    mouse.modifiers & Qt.ControlModifier);
                                        if (mouse.button === Qt.RightButton) {
                                            entryMenu.popup();
                                        } else if (mouse.modifiers & Qt.ControlModifier) {
                                            // selection only
                                        }
                                    }
                                    onDoubleClicked: root.openPath(gridCell.filePath, gridCell.isDir)
                                }
                            }
                        }

                        ListView {
                            id: listView
                            anchors.fill: parent
                            anchors.margins: Spacing.sm
                            visible: root.viewMode === "list" && !root.searchActive()
                            clip: true
                            model: dir
                            boundsBehavior: Flickable.StopAtBounds
                            spacing: 1

                            header: Item {
                                width: listView.width
                                height: 28

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: Spacing.md
                                    spacing: Spacing.md

                                    Text { width: listView.width * 0.5; text: qsTr("Ad"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                                    Text { width: 110; text: qsTr("Boyut"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                                    Text { width: 140; text: qsTr("Değiştirilme"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                                }
                            }

                            delegate: Item {
                                id: listRow
                                required property string name
                                required property string filePath
                                required property bool isDir
                                required property string iconName
                                required property string sizeText
                                required property string modifiedText

                                width: listView.width
                                height: 32

                                Rectangle {
                                    anchors.fill: parent
                                    radius: Radius.small
                                    color: root.selectedPaths.indexOf(listRow.filePath) >= 0
                                           ? Theme.accentSoft
                                           : (rowHover.containsMouse ? Theme.surfaceHover : "transparent")
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: Spacing.md
                                    anchors.rightMargin: Spacing.md
                                    spacing: Spacing.md

                                    TagIcon {
                                        anchors.verticalCenter: parent.verticalCenter
                                        name: listRow.iconName
                                        size: 16
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: listView.width * 0.5
                                        text: listRow.name
                                        color: Theme.text
                                        font.family: Typography.family
                                        font.pixelSize: Typography.body
                                        elide: Text.ElideMiddle
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 110
                                        text: listRow.isDir ? qsTr("klasör") : listRow.sizeText
                                        color: Theme.textMuted
                                        font.family: Typography.family
                                        font.pixelSize: Typography.caption
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 140
                                        text: listRow.modifiedText
                                        color: Theme.textMuted
                                        font.family: Typography.family
                                        font.pixelSize: Typography.caption
                                    }
                                }

                                MouseArea {
                                    id: rowHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: mouse => {
                                        root.select(listRow.filePath,
                                                    mouse.modifiers & Qt.ControlModifier);
                                        if (mouse.button === Qt.RightButton)
                                            entryMenu.popup();
                                    }
                                    onDoubleClicked: root.openPath(listRow.filePath, listRow.isDir)
                                }
                            }
                        }

                        // Search results
                        ListView {
                            id: searchResults
                            anchors.fill: parent
                            anchors.margins: Spacing.md
                            visible: root.searchActive()
                            clip: true
                            model: FileSearch.results
                            boundsBehavior: Flickable.StopAtBounds
                            spacing: 2

                            delegate: TagListRow {
                                required property var modelData
                                width: searchResults.width
                                icon: modelData.iconName
                                title: modelData.name
                                subtitle: modelData.parent
                                onClicked: {
                                    if (modelData.isDir)
                                        root.navigate(modelData.filePath, false);
                                    else
                                        root.openPath(modelData.filePath, false);
                                }
                            }
                        }

                        TagEmptyState {
                            anchors.centerIn: parent
                            visible: dir.count === 0 && !root.searchActive()
                            icon: "folder-open"
                            title: qsTr("Bu klasör boş")
                            message: qsTr("Yeni bir klasör oluşturmak için araç çubuğundaki simgeyi kullanın.")
                        }

                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            acceptedButtons: Qt.RightButton
                            onClicked: backgroundMenu.popup()
                        }
                    }

                    // Properties pane
                    Rectangle {
                        width: root.propertiesOpen ? 272 : 0
                        height: parent.height
                        visible: width > 0
                        color: Theme.surface

                        Rectangle {
                            anchors.left: parent.left
                            width: 1
                            height: parent.height
                            color: Theme.border
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: Spacing.md
                            spacing: Spacing.sm

                            Text {
                                text: qsTr("Özellikler")
                                color: Theme.textDim
                                font.family: Typography.family
                                font.pixelSize: Typography.body
                                font.weight: Typography.medium
                            }

                            TagEmptyState {
                                width: parent.width
                                visible: !root.hasSelection
                                icon: "info"
                                title: qsTr("Seçim yok")
                                message: qsTr("Bir dosya seçin; boyut, tür ve önizleme burada görünür.")
                            }

                            Item {
                                width: parent.width
                                height: 120
                                visible: root.preview.kind === "image"

                                Image {
                                    anchors.fill: parent
                                    source: root.preview.url || ""
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }
                            }

                            Text {
                                width: parent.width
                                visible: root.preview.kind === "text"
                                text: (root.preview.text || "").substring(0, 600)
                                color: Theme.textMuted
                                font.family: Typography.monoFamily
                                font.pixelSize: Typography.caption - 1
                                wrapMode: Text.WrapAnywhere
                                maximumLineCount: 12
                                elide: Text.ElideRight
                            }

                            Repeater {
                                model: root.hasSelection ? [
                                    { label: qsTr("Ad"), value: root.preview.name || "" },
                                    { label: qsTr("Tür"), value: root.preview.mime || root.preview.kind || "" },
                                    { label: qsTr("Boyut"), value: root.preview.size || "" },
                                    { label: qsTr("Değiştirilme"), value: root.preview.modified || "" },
                                    { label: qsTr("Yol"), value: root.preview.path || "" }
                                ] : []

                                delegate: Column {
                                    required property var modelData
                                    width: parent.width
                                    spacing: 1

                                    Text {
                                        text: modelData.label
                                        color: Theme.textMuted
                                        font.family: Typography.family
                                        font.pixelSize: Typography.caption - 1
                                    }

                                    Text {
                                        width: parent.width
                                        text: modelData.value
                                        color: Theme.textDim
                                        font.family: Typography.family
                                        font.pixelSize: Typography.caption
                                        wrapMode: Text.WrapAnywhere
                                    }
                                }
                            }

                            Item { width: 1; height: Spacing.sm }

                            Row {
                                spacing: Spacing.xs
                                visible: root.hasSelection

                                TagButton {
                                    text: qsTr("Yeniden adlandır")
                                    icon: "edit"
                                    variant: TagButton.Ghost
                                    onClicked: root.startRename()
                                }
                            }

                            Row {
                                spacing: Spacing.xs
                                visible: root.hasSelection

                                TagButton {
                                    text: qsTr("Çöpe taşı")
                                    icon: "trash"
                                    variant: TagButton.Ghost
                                    onClicked: root.trashSelection()
                                }
                            }
                        }
                    }
                }

                // Status bar
                Rectangle {
                    width: parent.width
                    height: 29
                    color: Theme.surface

                    Rectangle {
                        anchors.top: parent.top
                        width: parent.width
                        height: 1
                        color: Theme.border
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Spacing.md
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("%1 öğe").arg(dir.count)
                              + (root.hasSelection ? qsTr(" · %1 seçili").arg(root.selectionCount) : "")
                        color: Theme.textMuted
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: Spacing.md
                        anchors.verticalCenter: parent.verticalCenter
                        text: FileOps.lastError()
                        color: Theme.danger
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                        width: parent.width * 0.45
                    }
                }
            }
        }
    }

    function searchActive() {
        return searchText.trim().length > 0;
    }

    onSearchTextChanged: {
        if (searchActive())
            FileSearch.start(dir.path, searchText, 60);
        else
            FileSearch.clear();
    }

    // --- menus ------------------------------------------------------------
    TagMenu {
        id: entryMenu
        items: [
            { label: qsTr("Aç"), icon: "app" },
            { separator: true },
            { label: qsTr("Kopyala"), icon: "copy", shortcut: "Ctrl+C" },
            { label: qsTr("Kes"), icon: "cut", shortcut: "Ctrl+X" },
            { label: qsTr("Yeniden adlandır"), icon: "edit", shortcut: "F2" },
            { separator: true },
            { label: qsTr("Çöpe taşı"), icon: "trash", danger: true, shortcut: "Del" },
            { label: qsTr("Kalıcı olarak sil"), icon: "trash-restore", danger: true,
              enabled: root.selectionCount > 0 }
        ]
        onTriggered: index => {
            if (index === 0 && root.selectionCount === 1)
                root.openPath(root.selectedPaths[0], dir.at(dir.indexOf(root.selectedPaths[0])).isDir);
            else if (index === 2)
                root.copySelection();
            else if (index === 3)
                root.cutSelection();
            else if (index === 4)
                root.startRename();
            else if (index === 6)
                root.trashSelection();
            else if (index === 7) {
                root.pendingDelete = root.selectedPaths.join("\n");
                confirmDelete.open();
            }
        }
    }

    TagMenu {
        id: backgroundMenu
        items: [
            { label: qsTr("Yeni klasör"), icon: "folder-plus" },
            { label: qsTr("Yapıştır"), icon: "paste", enabled: root.clipboardPaths.length > 0 },
            { separator: true },
            { label: qsTr("Yenile"), icon: "refresh" }
        ]
        onTriggered: index => {
            if (index === 0)
                root.newFolder();
            else if (index === 1)
                root.paste();
            else if (index === 3)
                dir.refresh();
        }
    }

    // --- dialogs ----------------------------------------------------------
    Popup {
        id: renameDialog
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 380
        padding: Spacing.lg
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: Theme.card
            radius: Radius.medium
            border.width: 1
            border.color: Theme.borderStrong
        }

        contentItem: Column {
            spacing: Spacing.md

            Text {
                text: qsTr("Yeniden adlandır")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.semiBold
            }

            Rectangle {
                width: parent.width
                height: 34
                radius: Radius.small
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.accent

                TextInput {
                    id: renameInput
                    anchors.fill: parent
                    anchors.leftMargin: Spacing.sm
                    anchors.rightMargin: Spacing.sm
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.body
                    text: root.renamingText
                    selectByMouse: true
                    onTextChanged: root.renamingText = text

                    Keys.onReturnPressed: {
                        root.applyRename();
                        renameDialog.close();
                    }
                }
            }

            Row {
                spacing: Spacing.sm

                TagButton {
                    text: qsTr("Vazgeç")
                    variant: TagButton.Ghost
                    onClicked: renameDialog.close()
                }

                TagButton {
                    text: qsTr("Kaydet")
                    onClicked: {
                        root.applyRename();
                        renameDialog.close();
                    }
                }
            }
        }
    }

    Popup {
        id: confirmDelete
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 400
        padding: Spacing.lg
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: Theme.card
            radius: Radius.medium
            border.width: 1
            border.color: Theme.danger
        }

        contentItem: Column {
            spacing: Spacing.md

            Text {
                text: qsTr("Kalıcı olarak silinsin mi?")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.semiBold
            }

            Text {
                width: parent.width
                text: qsTr("%1 öğe geri alınamayacak şekilde silinecek. Çöp kutusu kullanılamıyor.")
                      .arg(root.pendingDelete.split("\n").length)
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                wrapMode: Text.Wrap
            }

            Row {
                spacing: Spacing.sm

                TagButton {
                    text: qsTr("Vazgeç")
                    variant: TagButton.Ghost
                    onClicked: {
                        root.pendingDelete = "";
                        confirmDelete.close();
                    }
                }

                TagButton {
                    text: qsTr("Kalıcı olarak sil")
                    variant: TagButton.Danger
                    onClicked: {
                        root.deletePermanently();
                        confirmDelete.close();
                    }
                }
            }
        }
    }

    Shortcut { sequence: "Ctrl+C"; onActivated: root.copySelection() }
    Shortcut { sequence: "Ctrl+X"; onActivated: root.cutSelection() }
    Shortcut { sequence: "Ctrl+V"; onActivated: root.paste() }
    Shortcut { sequence: "F2"; onActivated: root.startRename() }
    Shortcut { sequence: "Delete"; onActivated: root.trashSelection() }
    Shortcut { sequence: "Backspace"; onActivated: root.goUp() }
    Shortcut { sequence: "Ctrl+H"; onActivated: dir.showHidden = !dir.showHidden }
    Shortcut { sequence: "Ctrl+F"; onActivated: searchField.forceActiveFocus() }
}
