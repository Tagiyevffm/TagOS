import QtQuick
import TagOS.Design 1.0
import TagOS.System 1.0

// TagSettings — one window, six real pages. Every control writes through
// SettingsStore or a system adapter; there are no decorative switches.
TagAppWindow {
    id: root

    iconName: "tag-settings"
    titleText: qsTr("Ayarlar")
    subtitle: "TagOS " + SessionInfo.tagosVersion
    width: 1040
    height: 720

    readonly property var sections: [
        { label: qsTr("Görünüm"), icon: "palette", page: "AppearancePage.qml" },
        { label: qsTr("Ağ"), icon: "wifi", page: "NetworkPage.qml" },
        { label: qsTr("Ses"), icon: "volume", page: "SoundPage.qml" },
        { label: qsTr("Güç"), icon: "power-profile", page: "PowerPage.qml" },
        { label: qsTr("Kullanıcı"), icon: "user", page: "UserPage.qml" },
        { label: qsTr("Sistem"), icon: "cpu", page: "SystemPage.qml" }
    ]

    property int currentIndex: 0

    Row {
        anchors.fill: parent

        // --- navigation ---------------------------------------------------
        Rectangle {
            width: 220
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
                anchors.top: parent.top
                anchors.topMargin: Spacing.md
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Spacing.sm

                Repeater {
                    model: root.sections

                    delegate: Item {
                        id: navItem
                        required property var modelData
                        required property int index

                        width: parent.width
                        height: 40

                        Rectangle {
                            anchors.fill: parent
                            radius: Radius.small
                            color: root.currentIndex === navItem.index
                                   ? Theme.accentSoft
                                   : (navHover.containsMouse ? Theme.surfaceHover : "transparent")
                            border.width: root.currentIndex === navItem.index ? 1 : 0
                            border.color: Theme.accent

                            Behavior on color { ColorAnimation { duration: Motion.fast } }
                        }

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: Spacing.md
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Spacing.md

                            TagIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: navItem.modelData.icon
                                size: 16
                                opacity: root.currentIndex === navItem.index ? 1.0 : 0.7
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: navItem.modelData.label
                                color: root.currentIndex === navItem.index ? Theme.text : Theme.textDim
                                font.family: Typography.family
                                font.pixelSize: Typography.body
                                font.weight: root.currentIndex === navItem.index
                                             ? Typography.semiBold : Typography.regular
                            }
                        }

                        MouseArea {
                            id: navHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentIndex = navItem.index
                        }
                    }
                }
            }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Spacing.md
                text: SessionInfo.prettyOsName
                color: Theme.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption - 1
                elide: Text.ElideRight
            }
        }

        // --- page ---------------------------------------------------------
        Flickable {
            id: flick
            width: parent.width - 220
            height: parent.height
            clip: true
            contentWidth: width
            contentHeight: loader.item ? loader.item.implicitHeight + Spacing.xxl * 2 : height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            Loader {
                id: loader
                x: Spacing.xxl
                y: Spacing.xl
                width: flick.width - Spacing.xxl * 2
                source: "pages/" + root.sections[root.currentIndex].page
            }

            Rectangle {
                width: 4
                radius: 2
                height: flick.visibleArea.heightRatio * flick.height
                x: flick.width - 10
                y: flick.visibleArea.yPosition * flick.height
                color: Theme.borderStrong
                visible: flick.visibleArea.heightRatio < 1.0
            }
        }
    }
}
