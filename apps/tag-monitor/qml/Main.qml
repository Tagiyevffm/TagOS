pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import TagOS.Design 1.0
import TagOS.Flow 1.0
import TagOS.System 1.0

// TagMonitor — live system monitor.
//
// Genel: CPU/RAM/disk rings + history. Süreçler: real /proc table with SIGTERM.
// Ağ: throughput from /proc/net/dev + thermal zone. Nothing is simulated.
TagAppWindow {
    id: root

    iconName: "tag-monitor"
    titleText: qsTr("TagMonitor")
    subtitle: ProcessList.available ? ProcessList.summary : qsTr("sınırlı izleme")
    width: 1040
    height: 700

    FlowClient { id: flow; pollIntervalMs: 1500 }

    property int tab: 0
    property string processFilter: ""
    property var pendingKill: null

    readonly property var tabs: [qsTr("Genel"), qsTr("Süreçler"), qsTr("Ağ")]

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: {
            if (root.tab === 1)
                ProcessList.refresh();
            if (root.tab === 2) {
                NetworkSampler.refresh();
                ThermalReader.refresh();
            }
        }
    }

    Column {
        anchors.fill: parent

        // Tab bar
        Rectangle {
            width: parent.width
            height: 44
            color: Theme.surface

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Spacing.md
                anchors.verticalCenter: parent.verticalCenter
                spacing: Spacing.xs

                Repeater {
                    model: root.tabs

                    delegate: Item {
                        id: tabItem
                        required property string modelData
                        required property int index

                        width: tabLabel.implicitWidth + Spacing.xl
                        height: 30

                        Rectangle {
                            anchors.fill: parent
                            radius: Radius.small
                            color: root.tab === tabItem.index
                                   ? Theme.accentSoft
                                   : (tabHover.containsMouse ? Theme.surfaceHover : "transparent")
                            border.width: root.tab === tabItem.index ? 1 : 0
                            border.color: Theme.accent
                        }

                        Text {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: tabItem.modelData
                            color: root.tab === tabItem.index ? Theme.text : Theme.textDim
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                            font.weight: root.tab === tabItem.index
                                         ? Typography.semiBold : Typography.regular
                        }

                        MouseArea {
                            id: tabHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.tab = tabItem.index
                        }
                    }
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: Spacing.md
                anchors.verticalCenter: parent.verticalCenter
                spacing: Spacing.sm

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: flow.transportLabel
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.border
            }
        }

        // --- Genel ---------------------------------------------------------
        Item {
            width: parent.width
            height: parent.height - 44
            visible: root.tab === 0

            Column {
                anchors.fill: parent
                anchors.margins: Spacing.xl
                spacing: Spacing.lg

                Row {
                    spacing: Spacing.xl

                    TagRingGauge {
                        width: 132
                        height: 132
                        thickness: 9
                        valueFontSize: Typography.headline
                        value: flow.cpuPercent
                        label: "CPU"
                        caption: flow.cpuCount + " " + qsTr("iş parçacığı")
                        progressColor: Theme.accent
                    }

                    TagRingGauge {
                        width: 132
                        height: 132
                        thickness: 9
                        valueFontSize: Typography.headline
                        value: flow.memoryUsedPercent
                        label: "RAM"
                        caption: flow.memoryText
                        progressColor: Theme.accentHover
                    }

                    TagRingGauge {
                        width: 132
                        height: 132
                        thickness: 9
                        valueFontSize: Typography.headline
                        value: flow.diskUsedPercent
                        label: "Disk"
                        caption: flow.diskText
                        progressColor: Theme.info
                    }

                    TagRingGauge {
                        width: 132
                        height: 132
                        thickness: 9
                        valueFontSize: Typography.headline
                        value: ThermalReader.available ? ThermalReader.celsius : -1
                        label: qsTr("Sıcaklık")
                        caption: ThermalReader.available ? ThermalReader.label : qsTr("sensör yok")
                        progressColor: ThermalReader.celsius > 80 ? Theme.danger : Theme.success
                    }
                }

                TagCard {
                    width: parent.width
                    height: 236
                    padding: Spacing.lg

                    Column {
                        width: parent.width
                        spacing: Spacing.sm

                        Text {
                            text: qsTr("CPU geçmişi")
                            color: Theme.textDim
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                        }

                        TagSparkline {
                            width: parent.width
                            height: 78
                            values: flow.cpuHistory
                            lineColor: Theme.accent
                        }

                        Text {
                            text: qsTr("Bellek geçmişi")
                            color: Theme.textDim
                            font.family: Typography.family
                            font.pixelSize: Typography.body
                        }

                        TagSparkline {
                            width: parent.width
                            height: 52
                            values: flow.memoryHistory
                            lineColor: Theme.info
                        }
                    }
                }

                Grid {
                    width: parent.width
                    columns: 2
                    columnSpacing: Spacing.xxl
                    rowSpacing: Spacing.sm

                    Text { text: qsTr("Sistem"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: flow.distroName; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

                    Text { text: qsTr("Çekirdek"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: flow.kernelVersion; color: Theme.text; font.family: Typography.monoFamily; font.pixelSize: Typography.body }

                    Text { text: qsTr("Çalışma süresi"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: flow.uptimeText; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

                    Text { text: qsTr("Yük ortalaması"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: flow.loadText; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }

                    Text { text: qsTr("Ağ"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: NetworkSampler.available ? NetworkSampler.interfaceName : qsTr("arayüz yok"); color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body }
                }
            }
        }

        // --- Süreçler ------------------------------------------------------
        Item {
            width: parent.width
            height: parent.height - 44
            visible: root.tab === 1

            Column {
                anchors.fill: parent
                anchors.margins: Spacing.lg
                spacing: Spacing.sm

                Row {
                    width: parent.width
                    spacing: Spacing.md

                    Rectangle {
                        width: 260
                        height: 30
                        radius: Radius.small
                        color: Theme.surfaceAlt
                        border.width: 1
                        border.color: filterInput.activeFocus ? Theme.accent : Theme.border

                        TextInput {
                            id: filterInput
                            anchors.fill: parent
                            anchors.leftMargin: Spacing.sm
                            anchors.rightMargin: Spacing.sm
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.text
                            font.family: Typography.family
                            font.pixelSize: Typography.caption
                            onTextChanged: root.processFilter = text

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: filterInput.text.length === 0
                                text: qsTr("Süreç ara…")
                                color: Theme.textMuted
                                font: filterInput.font
                            }
                        }
                    }

                    TagButton {
                        text: qsTr("Yenile")
                        icon: "refresh"
                        variant: TagButton.Tonal
                        onClicked: ProcessList.refresh()
                    }

                    Item { width: parent.width - 460; height: 1 }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: ProcessList.summary
                        color: Theme.textMuted
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Theme.border }

                Item {
                    width: parent.width
                    height: 26

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Spacing.sm
                        spacing: Spacing.md

                        Text { width: 70; text: "PID"; color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                        Text { width: parent.width * 0.4; text: qsTr("Süreç"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                        Text { width: 90; text: "CPU"; color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                        Text { width: 110; text: qsTr("Bellek"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    }
                }

                ListView {
                    id: processView
                    width: parent.width
                    height: parent.height - 100
                    clip: true
                    model: ProcessList
                    boundsBehavior: Flickable.StopAtBounds
                    spacing: 1

                    delegate: Item {
                        id: processRow
                        required property int pid
                        required property string name
                        required property string command
                        required property string cpuText
                        required property string memoryText

                        width: processView.width
                        height: visible ? 30 : 0
                        visible: root.processFilter.length === 0
                                 || name.toLowerCase().indexOf(root.processFilter.toLowerCase()) >= 0
                                 || command.toLowerCase().indexOf(root.processFilter.toLowerCase()) >= 0

                        Rectangle {
                            anchors.fill: parent
                            radius: Radius.small
                            color: rowHover.containsMouse ? Theme.surfaceHover : "transparent"
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Spacing.sm
                            anchors.rightMargin: Spacing.sm
                            spacing: Spacing.md

                            Text { width: 70; anchors.verticalCenter: parent.verticalCenter; text: processRow.pid; color: Theme.textMuted; font.family: Typography.monoFamily; font.pixelSize: Typography.caption }
                            Text { width: processView.width * 0.4; anchors.verticalCenter: parent.verticalCenter; text: processRow.name; color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.body; elide: Text.ElideRight }
                            Text { width: 90; anchors.verticalCenter: parent.verticalCenter; text: processRow.cpuText; color: Theme.accent; font.family: Typography.monoFamily; font.pixelSize: Typography.caption }
                            Text { width: 110; anchors.verticalCenter: parent.verticalCenter; text: processRow.memoryText; color: Theme.textDim; font.family: Typography.monoFamily; font.pixelSize: Typography.caption }
                        }

                        MouseArea {
                            id: rowHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                root.pendingKill = { pid: processRow.pid, name: processRow.name };
                                killDialog.open();
                            }
                        }
                    }
                }

                TagEmptyState {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !ProcessList.available
                    icon: "process"
                    title: qsTr("/proc okunamıyor")
                    message: qsTr("Süreç listesi Linux'ta /proc üzerinden okunur. Bu platformda desteklenmiyor.")
                }
            }
        }

        // --- Ağ ------------------------------------------------------------
        Item {
            width: parent.width
            height: parent.height - 44
            visible: root.tab === 2

            Column {
                anchors.fill: parent
                anchors.margins: Spacing.xl
                spacing: Spacing.lg

                Text {
                    text: qsTr("Ağ")
                    color: Theme.text
                    font.family: Typography.family
                    font.pixelSize: Typography.title
                    font.weight: Typography.semiBold
                }

                Grid {
                    columns: 2
                    columnSpacing: Spacing.xxxl
                    rowSpacing: Spacing.md

                    Text { text: qsTr("Arayüz"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: NetworkSampler.available ? (NetworkSampler.interfaceName || qsTr("etkin değil")) : qsTr("yok"); color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.subtitle }

                    Text { text: qsTr("İndirme"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: NetworkSampler.rxRateText + "  ·  " + qsTr("toplam %1").arg(NetworkSampler.rxTotalText); color: Theme.text; font.family: Typography.monoFamily; font.pixelSize: Typography.subtitle }

                    Text { text: qsTr("Yükleme"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: NetworkSampler.txRateText + "  ·  " + qsTr("toplam %1").arg(NetworkSampler.txTotalText); color: Theme.text; font.family: Typography.monoFamily; font.pixelSize: Typography.subtitle }

                    Text { text: qsTr("Sıcaklık"); color: Theme.textMuted; font.family: Typography.family; font.pixelSize: Typography.caption }
                    Text { text: ThermalReader.available ? ThermalReader.text + " · " + ThermalReader.label : qsTr("sensör yok"); color: Theme.text; font.family: Typography.family; font.pixelSize: Typography.subtitle }
                }

                TagEmptyState {
                    width: 420
                    visible: !NetworkSampler.available
                    icon: "globe"
                    title: qsTr("Ağ istatistiği yok")
                    message: qsTr("/proc/net/dev okunamadı; bu ekran yalnızca gerçek veri gösterir.")
                }

                Item { width: 1; height: 1 }

                Text {
                    width: parent.width * 0.7
                    text: qsTr("Süreç sonlandırmak için Süreçler sekmesinde bir satıra sağ tıklayın.")
                    color: Theme.textMuted
                    font.family: Typography.family
                    font.pixelSize: Typography.caption
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Popup {
        id: killDialog
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 380
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
                text: qsTr("Süreç sonlandırılsın mı?")
                color: Theme.text
                font.family: Typography.family
                font.pixelSize: Typography.subtitle
                font.weight: Typography.semiBold
            }

            Text {
                width: parent.width
                text: root.pendingKill
                      ? qsTr("%1 (pid %2) sürecine SIGTERM gönderilecek.").arg(root.pendingKill.name).arg(root.pendingKill.pid)
                      : ""
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
                    onClicked: killDialog.close()
                }

                TagButton {
                    text: qsTr("Sonlandır")
                    variant: TagButton.Danger
                    onClicked: {
                        if (root.pendingKill)
                            ProcessList.kill(root.pendingKill.pid);
                        root.pendingKill = null;
                        killDialog.close();
                        ProcessList.refresh();
                    }
                }
            }
        }
    }
}
