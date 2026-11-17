pragma ComponentBehavior: Bound

import QtQuick
import TagOS.Design 1.0
import TagOS.System 1.0

// Full-screen application launcher (Ctrl+Space / Super / top-panel search).
Item {
    id: root

    property var catalog: null
    property string query: ""
    property bool open: false

    signal closeRequested()

    visible: opacity > 0
    opacity: open ? 1 : 0

    Behavior on opacity { NumberAnimation { duration: Motion.normal; easing.type: Motion.easing } }

    function show() {
        open = true;
        query = "";
        searchInput.forceActiveFocus();
    }

    function hide() {
        open = false;
        searchInput.focus = false;
    }

    function toggle() {
        if (open)
            hide();
        else
            show();
    }

    function activate(app) {
        if (!app || !app.available) {
            hint.visible = true;
            hintTimer.restart();
            return;
        }
        AppLauncher.launch(app.exec);
        hide();
    }

    // Dimmed backdrop; clicking it closes the launcher.
    Rectangle {
        anchors.fill: parent
        color: "#C40A0807"

        MouseArea {
            anchors.fill: parent
            onClicked: root.hide()
        }
    }

    TagCard {
        id: panel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 110
        width: Math.min(880, parent.width - Spacing.xxxl * 2)
        height: 460
        padding: Spacing.xl
        borderColor: Theme.borderStrong
        surfaceColor: Theme.bgElevated

        scale: root.open ? 1.0 : 0.97
        Behavior on scale { NumberAnimation { duration: Motion.normal; easing.type: Motion.easing } }

        Column {
            anchors.fill: parent
            spacing: Spacing.lg

            Rectangle {
                width: parent.width
                height: 46
                radius: Radius.medium
                color: Theme.surface
                border.width: 1
                border.color: searchInput.activeFocus ? Theme.accent : Theme.border

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Spacing.lg
                    anchors.rightMargin: Spacing.lg
                    spacing: Spacing.md

                    TagIcon {
                        name: "search"
                        size: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextInput {
                        id: searchInput
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 60
                        height: parent.height
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.text
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.accentInk
                        font.family: Typography.family
                        font.pixelSize: Typography.body
                        text: root.query
                        onTextChanged: root.query = text
                        Keys.onEscapePressed: root.hide()
                        Keys.onReturnPressed: root.activate(root.catalog.filtered(root.query)[0])
                        Keys.onEnterPressed: root.activate(root.catalog.filtered(root.query)[0])

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: searchInput.text.length === 0
                            text: qsTr("Uygulama, dosya veya komut ara…")
                            color: Theme.textMuted
                            font: searchInput.font
                        }
                    }
                }
            }

            GridView {
                id: grid
                width: parent.width
                height: parent.height - 46 - Spacing.lg - Spacing.md
                cellWidth: width / 5
                cellHeight: 104
                clip: true
                model: root.catalog ? root.catalog.filtered(root.query) : []
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: tile
                    required property var modelData
                    width: grid.cellWidth
                    height: grid.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: Spacing.xs
                        radius: Radius.medium
                        color: area.containsMouse ? Theme.surfaceHover : "transparent"
                        border.width: area.containsMouse ? 1 : 0
                        border.color: Theme.border
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: Spacing.sm

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 48
                            height: 48
                            radius: Radius.medium
                            color: tile.modelData.available ? Theme.surfaceAlt : Theme.surface
                            border.width: 1
                            border.color: tile.modelData.available ? Theme.borderStrong : Theme.border

                            TagIcon {
                                anchors.centerIn: parent
                                name: tile.modelData.icon
                                size: 24
                                dim: tile.modelData.available ? 1.0 : 0.35
                            }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: tile.modelData.name
                            color: tile.modelData.available ? Theme.text : Theme.textMuted
                            font.family: Typography.family
                            font.pixelSize: Typography.caption
                            font.weight: Typography.medium
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: tile.modelData.available ? tile.modelData.category : qsTr("Faz %1").arg(tile.modelData.phase)
                            color: Theme.textMuted
                            font.family: Typography.family
                            font.pixelSize: Typography.caption - 2
                        }
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: tile.modelData.available ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                        onClicked: root.activate(tile.modelData)
                    }
                }
            }

            Text {
                id: hint
                visible: false
                text: qsTr("Bu uygulama henüz geliştirilmedi — yol haritasında hangi fazda olduğunu launcher'da görebilirsin.")
                color: Theme.warning
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }
        }
    }

    Timer {
        id: hintTimer
        interval: 2600
        onTriggered: hint.visible = false
    }
}
