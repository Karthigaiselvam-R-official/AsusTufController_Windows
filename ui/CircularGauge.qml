import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 220
    height: 220
    
    // API
    property real value: 0
    property real maxValue: 100
    property string text: "0"
    property string subText: ""
    property color traceColor: "#1a1a1a"
    property color progressColor: "#0078d4"
    property int strokeWidth: 14
    property var appTheme: null
    
    // Safe theme accessors
    property bool _isDark: appTheme ? appTheme.isDark : true
    property color _surface: appTheme ? appTheme.surface : (_isDark ? "#1a1a1a" : "#f5f5f5")
    property color _border: appTheme ? appTheme.border : (_isDark ? "#2a2a2a" : "#d0d0d0")
    property color _textPrimary: appTheme ? appTheme.textPrimary : (_isDark ? "#ffffff" : "#0a0a0a")
    property color _textSecondary: appTheme ? appTheme.textSecondary : (_isDark ? "#b0b0b0" : "#3a3a3a")
    property int _fontSizeSmall: appTheme ? appTheme.fontSizeSmall : 12
    property int _fontWeightBold: appTheme ? appTheme.fontWeightBold : Font.Bold
    property int _space2: appTheme ? appTheme.space2 : 8
    
    // Animated value
    property real animatedValue: 0
    
    Behavior on animatedValue {
        SpringAnimation {
            spring: 2.8
            damping: 0.28
            epsilon: 0.01
        }
    }
    
    onValueChanged: animatedValue = value
    Component.onCompleted: animatedValue = value
    
    // Pulsing glow
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.86
        height: width
        radius: width / 2
        color: "transparent"
        border.color: root.progressColor
        border.width: 2
        opacity: 0.10
        
        SequentialAnimation on scale {
            loops: Animation.Infinite
            NumberAnimation { from: 0.94; to: 1.06; duration: 3500; easing.type: Easing.InOutSine }
            NumberAnimation { from: 1.06; to: 0.94; duration: 3500; easing.type: Easing.InOutSine }
        }
    }
    
    // Canvas gauge
    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        renderStrategy: Canvas.Threaded
        
        onPaint: {
            var ctx = getContext("2d")
            if (!ctx) return
            
            ctx.clearRect(0, 0, width, height)
            
            var cx = width / 2
            var cy = height / 2
            var radius = (Math.min(width, height) / 2) - root.strokeWidth - 14
            
            var startAngle = 0.70 * Math.PI
            var endAngle = 2.30 * Math.PI
            var sweepAngle = endAngle - startAngle
            
            var progress = Math.min(1.0, root.animatedValue / root.maxValue)
            var progressAngle = startAngle + (sweepAngle * progress)
            
            // Shadow for depth
            if (appTheme && !_isDark) {
                ctx.beginPath()
                ctx.arc(cx, cy + 3, radius, 0, 2 * Math.PI)
                ctx.fillStyle = Qt.rgba(0, 0, 0, 0.04)
                ctx.fill()
            }
            
            // Background track (shows full arc path)
            ctx.beginPath()
            ctx.arc(cx, cy, radius, startAngle, endAngle)
            ctx.lineWidth = root.strokeWidth + 4
            ctx.strokeStyle = _isDark ? Qt.rgba(0.25, 0.25, 0.25, 0.8) : Qt.rgba(0.80, 0.80, 0.80, 0.8)
            ctx.lineCap = "round"
            ctx.stroke()
            
            // Progress
            if (progress > 0.01) {
                var progressGrad = ctx.createLinearGradient(0, 0, width, height)
                progressGrad.addColorStop(0, root.progressColor)
                progressGrad.addColorStop(0.5, Qt.lighter(root.progressColor, 1.5))
                progressGrad.addColorStop(1, root.progressColor)
                
                ctx.beginPath()
                ctx.arc(cx, cy, radius, startAngle, progressAngle)
                ctx.lineWidth = root.strokeWidth
                ctx.strokeStyle = progressGrad
                ctx.lineCap = "round"
                ctx.stroke()
                
                // Glow
                ctx.globalAlpha = 0.30
                ctx.lineWidth = root.strokeWidth + 6
                ctx.stroke()
                ctx.globalAlpha = 1.0
                
                // Endpoint dot
                var dotX = cx + radius * Math.cos(progressAngle)
                var dotY = cy + radius * Math.sin(progressAngle)
                
                ctx.beginPath()
                ctx.arc(dotX, dotY, 11, 0, 2 * Math.PI)
                ctx.fillStyle = "#ffffff"
                ctx.fill()
                
                ctx.beginPath()
                ctx.arc(dotX, dotY, 9, 0, 2 * Math.PI)
                ctx.fillStyle = root.progressColor
                ctx.fill()
            }
            
            // Ticks - solid color for visibility
            ctx.strokeStyle = _isDark ? "#666666" : "#999999"
            ctx.lineWidth = 3
            for (var i = 0; i <= 10; i++) {
                var tickAngle = startAngle + (sweepAngle * i / 10)
                var major = (i % 5 === 0)
                var tickR1 = radius + root.strokeWidth / 2 + 10
                var tickR2 = tickR1 + (major ? 9 : 5)
                
                ctx.beginPath()
                ctx.moveTo(cx + tickR1 * Math.cos(tickAngle), cy + tickR1 * Math.sin(tickAngle))
                ctx.lineTo(cx + tickR2 * Math.cos(tickAngle), cy + tickR2 * Math.sin(tickAngle))
                ctx.stroke()
            }
        }
        
        Connections {
            target: root
            function onAnimatedValueChanged() { canvas.requestPaint() }
        }
        
        Component.onCompleted: requestPaint()
    }
    
    // Center display (no background circle)
    ColumnLayout {
        anchors.centerIn: parent
        spacing: _space2 - 3
        width: parent.width * 0.48
        
        // Value with glow
        Item {
            Layout.preferredHeight: 36
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            
            Text {
                id: mainText
                anchors.centerIn: parent
                width: parent.width
                text: root.text
                color: _textPrimary
                font.weight: _fontWeightBold
                font.pixelSize: {
                    // Responsive sizing
                    var len = text.length
                    if (len <= 5) return 28      // "0 RPM"
                    if (len <= 8) return 24      // "2400 RPM"
                    return 20                     // "10000 RPM"
                }
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                fontSizeMode: Text.Fit
                minimumPixelSize: 18
                elide: Text.ElideNone
            }
            
            Repeater {
                model: 2
                Text {
                    anchors.centerIn: parent
                    width: mainText.width
                    text: mainText.text
                    color: root.progressColor
                    font: mainText.font
                    opacity: 0.22 - index * 0.10
                    scale: 1.0 + index * 0.015
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 18
                }
            }
        }
        
        Text {
            text: root.subText
            color: _textSecondary
            font.pixelSize: _fontSizeSmall - 1
            font.weight: _fontWeightBold
            font.letterSpacing: 2.2
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
