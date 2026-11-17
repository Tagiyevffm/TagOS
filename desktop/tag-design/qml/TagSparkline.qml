import QtQuick

// Area sparkline fed directly by FlowClient.cpuHistory / memoryHistory.
Item {
    id: root

    property var values: []           // array of 0..100 numbers, oldest first
    property color lineColor: Theme.accent
    property real maxValue: 100
    property bool fill: true
    property int lineWidth: 2

    implicitWidth: 240
    implicitHeight: 56

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        renderStrategy: Canvas.Cooperative

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();

            const data = root.values || [];
            const count = data.length;
            if (count < 2)
                return;

            const w = width;
            const h = height;
            const stepX = w / (count - 1);
            const scale = Math.max(1, root.maxValue);

            const points = [];
            for (let i = 0; i < count; ++i) {
                const v = Math.max(0, Math.min(scale, data[i]));
                points.push({ x: i * stepX, y: h - (v / scale) * (h - root.lineWidth) - root.lineWidth / 2 });
            }

            if (root.fill) {
                const gradient = ctx.createLinearGradient(0, 0, 0, h);
                gradient.addColorStop(0, Qt.rgba(root.lineColor.r, root.lineColor.g, root.lineColor.b, 0.30));
                gradient.addColorStop(1, Qt.rgba(root.lineColor.r, root.lineColor.g, root.lineColor.b, 0.0));

                ctx.beginPath();
                ctx.moveTo(0, h);
                for (const p of points)
                    ctx.lineTo(p.x, p.y);
                ctx.lineTo(w, h);
                ctx.closePath();
                ctx.fillStyle = gradient;
                ctx.fill();
            }

            ctx.beginPath();
            ctx.lineWidth = root.lineWidth;
            ctx.lineJoin = "round";
            ctx.lineCap = "round";
            ctx.strokeStyle = root.lineColor;
            ctx.moveTo(points[0].x, points[0].y);
            for (let i = 1; i < points.length; ++i)
                ctx.lineTo(points[i].x, points[i].y);
            ctx.stroke();
        }

        Connections {
            target: root
            function onValuesChanged() { canvas.requestPaint(); }
            function onLineColorChanged() { canvas.requestPaint(); }
        }

        Component.onCompleted: requestPaint()
    }
}
