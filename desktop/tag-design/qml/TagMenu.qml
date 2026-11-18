import QtQuick
import QtQuick.Controls

// TagOS context / dropdown menu.
//
//   TagMenu {
//       items: [{ label: "Aç", icon: "app" }, { separator: true }, { label: "Sil", danger: true }]
//       onTriggered: index => …
//   }
Popup {
    id: root

    property var items: []
    property int currentIndex: -1
    property int menuWidth: 224

    signal triggered(int index)
    signal hovered(int index)

    padding: 6
    width: menuWidth
    implicitHeight: column.implicitHeight + padding * 2
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    modal: false

    background: Rectangle {
        color: Theme.card
        radius: Radius.medium
        border.width: 1
        border.color: Theme.borderStrong
    }

    contentItem: Column {
        id: column
        spacing: 1

        Repeater {
            model: root.items

            delegate: Item {
                id: row
                required property var modelData
                required property int index

                width: column.width
                height: row.modelData.separator ? 9 : 34
                visible: !row.modelData.hidden

                Rectangle {
                    visible: row.modelData.separator === true
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: Spacing.sm
                    anchors.rightMargin: Spacing.sm
                    height: 1
                    color: Theme.border
                }

                Rectangle {
                    visible: row.modelData.separator !== true
                    anchors.fill: parent
                    radius: Radius.small
                    color: mouse.containsMouse && enabledRow ? Theme.surfaceHover : "transparent"
                    border.width: row.modelData.checked === true ? 1 : 0
                    border.color: Theme.accent
                }

                readonly property bool enabledRow: row.modelData.enabled !== false

                Row {
                    visible: row.modelData.separator !== true
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Spacing.sm
                    anchors.right: parent.right
                    anchors.rightMargin: Spacing.sm
                    spacing: Spacing.sm
                    opacity: enabledRow ? 1.0 : 0.4

                    TagIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (row.modelData.icon || "").length > 0
                        name: row.modelData.icon || "grid"
                        size: 16
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - (row.modelData.shortcut || "").length * 7 - Spacing.xl
                        text: row.modelData.label || ""
                        color: row.modelData.danger ? Theme.danger : Theme.text
                        font.family: Typography.family
                        font.pixelSize: Typography.small
                        elide: Text.ElideRight
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (row.modelData.shortcut || "").length > 0
                        text: row.modelData.shortcut || ""
                        color: Theme.textMuted
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: row.modelData.separator !== true && enabledRow
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.close();
                        root.triggered(row.index);
                    }
                    onContainsMouseChanged: if (containsMouse)
                        root.hovered(row.index)
                }
            }
        }
    }
}
