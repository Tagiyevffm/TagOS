import QtQuick

// Ring gauge used by TagMonitor and the control centre (CPU / RAM / disk).
Item {
    id: root

    property real value: 0            // 0..100
    property string label: ""
    property string caption: ""
    property color progressColor: Theme.accent
    property color trackColor: Theme.surfaceHover
    property int thickness: 7
    property int valueFontSize: Typography.title

    implicitWidth: 96
    implicitHeight: 96

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        renderStrategy: Canvas.Cooperative

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();

            const size = Math.min(width, height);
            const center = size / 2;
            const lineWidth = root.thickness;
            const radius = center - lineWidth / 2 - 1;
            const startAngle = -Math.PI / 2;
            const sweep = Math.PI * 2 * Math.max(0, Math.min(100, root.value)) / 100;

            ctx.lineWidth = lineWidth;
            ctx.lineCap = "round";

            ctx.beginPath();
            ctx.strokeStyle = root.trackColor;
            ctx.arc(center, center, radius, 0, Math.PI * 2, false);
            ctx.stroke();

            if (sweep > 0.001) {
                ctx.beginPath();
                ctx.strokeStyle = root.progressColor;
                ctx.arc(center, center, radius, startAngle, startAngle + sweep, false);
                ctx.stroke();
            }
        }

        Connections {
            target: root
            function onValueChanged() { canvas.requestPaint(); }
            function onProgressColorChanged() { canvas.requestPaint(); }
        }

        Component.onCompleted: requestPaint()
    }

    Column {
        anchors.centerIn: parent
        spacing: 0

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.value < 0 ? "--" : Math.round(root.value) + "%"
            color: Theme.text
            font.family: Typography.family
            font.pixelSize: root.valueFontSize
            font.weight: Typography.bold
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.label.length > 0
            text: root.label
            color: Theme.textDim
            font.family: Typography.family
            font.pixelSize: Typography.caption
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.caption.length > 0
            text: root.caption
            color: Theme.textMuted
            font.family: Typography.family
            font.pixelSize: Typography.caption - 1
        }
    }
}
