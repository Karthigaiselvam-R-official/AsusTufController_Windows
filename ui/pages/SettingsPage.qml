import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Shapes 1.15
import AsusTufFanControl 1.0
import ".."
import "qrc:/AsusTufFanControl/ui/Translations.js" as Trans

Item {
    id: settingsPage
    
    property var theme
    property var monitor
    
    // Track current language for instant switching
    property string currentLang: LanguageController.currentLanguage
    
    // Helper function for instant translation
    function tr(text) {
        return Trans.tr(text, currentLang)
    }

    // Define SocialBtn component at root level
    component SocialBtn: Rectangle {
        id: btnRoot
        property string label
        property string btnColor
        property string hoverColor
        property string iconPath
        property real iconScale: 1.0
        signal clicked()
        
        Layout.fillWidth: true
        Layout.preferredHeight: 40 // Slightly taller for better touch
        Layout.maximumWidth: 160
        Layout.minimumWidth: 120
        radius: 8
        
        // Gradient or Solid logic - let's stick to solid with pop
        color: ma.containsMouse ? hoverColor : btnColor
        border.width: 1
        border.color: Qt.rgba(255,255,255,0.1)
        
        // Animations
        scale: ma.containsMouse ? 1.05 : 1.0
        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
        Behavior on color { ColorAnimation { duration: 150 } }
        
        MouseArea {
            id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
        
        RowLayout {
            anchors.centerIn: parent; spacing: 10
            
            // Icon Container
            Item {
                width: 20; height: 20
                Shape {
                    anchors.centerIn: parent
                    width: 24; height: 24 // Coordinate system size
                    scale: btnRoot.iconScale * (20/24) // Scale to fit 20px box
                    
                    ShapePath {
                        strokeColor: "transparent"
                        fillColor: "#ffffff"
                        PathSvg { path: btnRoot.iconPath }
                    }
                }
            }
            Text { 
                text: btnRoot.label; 
                color: "#fff"; 
                font.bold: true; 
                font.pixelSize: 13 
                font.family: "Segoe UI"
            }
        }
    }

    Flickable {
        id: pageFlickable

        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, mainColumn.implicitHeight + 60)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        
        // Vertical centering wrapper
        Item {
            id: centerWrapper
            width: pageFlickable.width
            height: Math.max(pageFlickable.height, mainColumn.implicitHeight + 60)
            
            ColumnLayout {
                id: mainColumn
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                
                width: Math.min(800, parent.width - 40)
                spacing: 24
        
                // ══════════════════════════════════════════════════════════════
                // SECTION 1: PAGE HEADER - Premium Gradient Card
                // ══════════════════════════════════════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    radius: 16
                    color: "transparent"
                    
                    // Gradient background
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: Qt.rgba(96/255, 125/255, 139/255, 0.15) }
                            GradientStop { position: 0.5; color: Qt.rgba(120/255, 144/255, 156/255, 0.1) }
                            GradientStop { position: 1.0; color: Qt.rgba(96/255, 125/255, 139/255, 0.15) }
                        }
                        border.width: 1
                        border.color: theme.isDark ? Qt.rgba(255,255,255,0.1) : Qt.rgba(0,0,0,0.05)
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 20
                    
                        // Animated Settings Gear Icon
                        Item {
                            width: 64; height: 64
                            Layout.alignment: Qt.AlignVCenter
                            
                            // Pulsating outer glow rings
                            Repeater {
                                model: 3
                                Rectangle {
                                    id: glowRing
                                    anchors.centerIn: parent
                                    width: 44 + (index * 8); height: width
                                    radius: width / 2
                                    color: "transparent"
                                    border.width: 2
                                    border.color: Qt.rgba(96/255, 125/255, 139/255, 0.3 - (index * 0.1))
                                    
                                    property real pulseScale: 1.0
                                    
                                    SequentialAnimation on pulseScale {
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 1.15; duration: 1000 + (index * 200); easing.type: Easing.InOutSine }
                                        NumberAnimation { to: 1.0; duration: 1000 + (index * 200); easing.type: Easing.InOutSine }
                                    }
                                    
                                    transform: Scale { 
                                        origin.x: glowRing.width / 2
                                        origin.y: glowRing.height / 2
                                        xScale: glowRing.pulseScale
                                        yScale: glowRing.pulseScale
                                    }
                                }
                            }
                            
                            // Animated Gear Icon
                            Canvas {
                                id: gearIcon
                                anchors.centerIn: parent
                                width: 40; height: 40
                                
                                property real rotation: 0
                                
                                NumberAnimation on rotation {
                                    running: true
                                    from: 0
                                    to: 360
                                    duration: 8000
                                    loops: Animation.Infinite
                                    easing.type: Easing.Linear
                                }
                                
                                onRotationChanged: requestPaint()
                                
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.translate(width/2, height/2);
                                    ctx.rotate(rotation * Math.PI / 180);
                                    
                                    // Gear color
                                    ctx.fillStyle = "#607d8b";
                                    ctx.strokeStyle = "#455a64";
                                    ctx.lineWidth = 1;
                                    
                                    // Draw gear teeth
                                    var outerRadius = 16;
                                    var innerRadius = 10;
                                    var teeth = 8;
                                    
                                    ctx.beginPath();
                                    for (var i = 0; i < teeth * 2; i++) {
                                        var angle = (i * Math.PI) / teeth;
                                        var radius = (i % 2 === 0) ? outerRadius : innerRadius;
                                        var x = Math.cos(angle) * radius;
                                        var y = Math.sin(angle) * radius;
                                        if (i === 0) ctx.moveTo(x, y);
                                        else ctx.lineTo(x, y);
                                    }
                                    ctx.closePath();
                                    ctx.fill();
                                    ctx.stroke();
                                    
                                    // Inner circle
                                    ctx.fillStyle = theme.isDark ? "#1a1a1a" : "#f0f0f0";
                                    ctx.beginPath();
                                    ctx.arc(0, 0, 5, 0, Math.PI * 2);
                                    ctx.fill();
                                }
                            }
                        }
                        
                        ColumnLayout {
                            spacing: 4
                            Layout.alignment: Qt.AlignVCenter
                            Text {
                                text: qsTr("SETTINGS")
                                color: "#607d8b"
                                font.pixelSize: 22
                                font.weight: Font.Bold
                                font.letterSpacing: 2
                            }
                            Text {
                                text: qsTr("Customize your experience")
                                color: Qt.rgba(96/255, 125/255, 139/255, 0.8)
                                font.pixelSize: 12
                                font.letterSpacing: 0.5
                            }
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        // Version Badge
                        Rectangle {
                            Layout.preferredWidth: Math.max(100, versionRow.implicitWidth + 30)
                            Layout.preferredHeight: 34
                            radius: 17
                            color: Qt.rgba(96/255, 125/255, 139/255, 0.15)
                            border.width: 1
                            border.color: Qt.rgba(96/255, 125/255, 139/255, 0.4)
                            
                            Row {
                                id: versionRow
                                anchors.centerIn: parent
                                spacing: 8
                                
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: "#607d8b"
                                }
                                Text {
                                    text: "v1.0.0"
                                    color: "#607d8b"
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.letterSpacing: 1.2
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                } // End Header
                
                // ══════════════════════════════════════════════════════════════
                // SECTION 2: APPEARANCE (Theme)
                // ══════════════════════════════════════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                    color: theme ? Qt.rgba(theme.surface.r, theme.surface.g, theme.surface.b, 0.9) : "#1a1a2e"
                    radius: 20
                    border.width: 1
                    border.color: theme && theme.isDark ? Qt.rgba(255,255,255,0.08) : Qt.rgba(0,0,0,0.08)
                    
                    // Subtle glow effect
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: 21
                        color: "transparent"
                        border.width: 2
                        border.color: Qt.rgba(255/255, 179/255, 0/255, 0.2) // Amber glow for Theme (or use generic blue? Amber looks good for Sun)
                        z: -1
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 20
                        
                        // Icon
                        Rectangle {
                            width: 48; height: 48
                            radius: 14
                            color: theme.isDark ? "#2d2d2d" : "#f0f0f0"
                            
                            Shape {
                                anchors.centerIn: parent
                                width: 24; height: 24
                                
                                ShapePath {
                                    strokeWidth: 0
                                    fillColor: "#e0c090" // Bright Tan Palette
                                    // Main Palette Shape
                                    PathSvg { path: "M12 3a9 9 0 0 0 0 18c.83 0 1.5-.67 1.5-1.5 0-.39-.15-.74-.39-1.01-.23-.26-.38-.61-.38-.99 0-.83.67-1.5 1.5-1.5H16c2.76 0 5-2.24 5-5 0-4.42-4.03-8-9-8z" }
                                }
                                // Vibrant Paint Spots
                                ShapePath { strokeWidth: 0; fillColor: "#ff4d4d"; PathSvg { path: "M6.5 10.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0z" } }
                                // Red
                                ShapePath { strokeWidth: 0; fillColor: "#ffd700"; PathSvg { path: "M9.5 6.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0z" } }
                                // Gold
                                ShapePath { strokeWidth: 0; fillColor: "#00bfff"; PathSvg { path: "M14.5 6.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0z" } }
                                // Sky Blue
                                ShapePath { strokeWidth: 0; fillColor: "#32cd32"; PathSvg { path: "M17.5 10.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0z" } }
                                // Lime Green
                            }
                        }
                        
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: qsTr("Appearance")
                                color: theme.textPrimary
                                font.bold: true
                                font.pixelSize: 16
                            }
                            Text {
                                text: theme.isDark ? qsTr("Dark Mode active") : qsTr("Light Mode active")
                                color: theme.textSecondary
                                font.pixelSize: 13
                            }
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        // Auto / System Mode Button
                         Rectangle {
                             id: autoBtn
                             width: Math.max(68, autoText.contentWidth + 24)
                             height: 34
                             radius: 12
                             // Gold (#FFC107) when active, transparent otherwise
                             color: ThemeController.themeMode === 0 ? "#FFC107" : "transparent"
                             border.width: 1
                             // Active: Gold outline. Inactive: Dark Grey (Dark Mode) or Medium Grey (Light Mode) for visibility
                             border.color: ThemeController.themeMode === 0 ? "#FFCA28" : (theme.isDark ? "#666" : "#888")
                             
                             scale: autoBtnMouse.pressed ? 0.95 : 1.0
                             Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                             Behavior on color { ColorAnimation { duration: 200 } }
                             
                             Text {
                                 id: autoText
                                 anchors.centerIn: parent
                                 text: qsTr("Auto")
                                 // Black text on Gold for better contrast, else normal theme text
                                 color: ThemeController.themeMode === 0 ? "#000000" : theme.textSecondary
                                 font.pixelSize: 12
                                 font.bold: true
                             }
                            
                            MouseArea {
                                id: autoBtnMouse
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (ThemeController.themeMode === 0) {
                                        // If System (Auto) is ON, switch to Manual (keep current theme)
                                        // 1=Light, 2=Dark
                                        ThemeController.setThemeMode(theme.isDark ? 2 : 1) 
                                    } else {
                                        // If Manual, switch to Auto
                                        ThemeController.setThemeMode(0) 
                                    }
                                }
                            }
                        }

                        // Custom Slider Switch (Manual Override)
                        Rectangle {
                            width: 64; height: 34
                            radius: 17
                            color: theme.isDark ? "#5E35B1" : "#e0e0e0" // Deep Purple for Dark Mode
                            border.width: 1
                            border.color: theme.isDark ? "transparent" : "#ccc"
                            opacity: ThemeController.themeMode === 0 ? 0.8 : 1.0 // Slightly dim if Auto
                            
                            Behavior on color { ColorAnimation { duration: 200 } }
                            
                            Rectangle {
                                x: theme.isDark ? 32 : 2
                                anchors.verticalCenter: parent.verticalCenter
                                width: 30; height: 30
                                radius: 15
                                color: "#ffffff"
                                
                                Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                                
                                Shape {
                                    anchors.centerIn: parent
                                    width: 24; height: 24
                                    scale: 0.65 
                                    
                                    ShapePath {
                                        strokeWidth: 0
                                        strokeColor: "transparent"
                                        fillColor: theme.isDark ? "#5E35B1" : "#FFB300"
                                        
                                        PathSvg { 
                                            path: theme.isDark 
                                                ? "M12 3c-4.97 0-9 4.03-9 9s4.03 9 9 9 9-4.03 9-9c0-.46-.04-.92-.1-1.36-.98 1.37-2.58 2.26-4.4 2.26-2.98 0-5.4-2.42-5.4-5.4 0-1.81.89-3.42 2.26-4.4-.44-.06-.9-.1-1.36-.1z"
                                                : "M6.99 11L3 15l3.99 4v-3h10v3l3.99-4L16.99 11v3h-10v-3zM12 7c-2.76 0-5 2.24-5 5s2.24 5 5 5 5-2.24 5-5-2.24-5-5-5zm0 8c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3z"
                                        } 
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // Always switch between Light(1) and Dark(2), never go back to System(0)
                                    // If currently Dark (Manual or Auto), go Light (Manual)
                                    // If currently Light (Manual or Auto), go Dark (Manual)
                                    ThemeController.setThemeMode(theme.isDark ? 1 : 2);
                                }
                            }
                        }
                    }
                }

                // ══════════════════════════════════════════════════════════════
                // SECTION 2B: TEMPERATURE UNITS
                // ══════════════════════════════════════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 88
                    color: theme ? Qt.rgba(theme.surface.r, theme.surface.g, theme.surface.b, 0.9) : "#1a1a2e"
                    radius: 20
                    border.width: 1
                    border.color: theme && theme.isDark ? Qt.rgba(255,255,255,0.08) : Qt.rgba(0,0,0,0.08)
                    
                    // Subtle glow effect
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: 21
                        color: "transparent"
                        border.width: 2
                        border.color: Qt.rgba(255/255, 68/255, 68/255, 0.2) // Red glow for Temperature
                        z: -1
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 20
                        
                        // Icon
                        Rectangle {
                            width: 48; height: 48
                            radius: 14
                            color: theme.isDark ? "#2d2d2d" : "#f0f0f0"
                            
                            Text {
                                anchors.centerIn: parent
                                text: "🌡️"
                                font.pixelSize: 24
                            }
                        }
                        
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: qsTr("Temperature Unit")
                                color: theme.textPrimary
                                font.bold: true
                                font.pixelSize: 16
                            }
                            Text {
                                text: ThemeController.tempUnit === 0 ? qsTr("Celsius (°C)") : qsTr("Fahrenheit (°F)")
                                color: theme.textSecondary
                                font.pixelSize: 13
                            }
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        // Unit Switcher
                        Rectangle {
                            width: 120; height: 34
                            radius: 17
                            color: theme.isDark ? "#333" : "#e0e0e0"
                            border.width: 1
                            border.color: theme.isDark ? "#444" : "#ccc"
                            
                            Row {
                                anchors.fill: parent
                                
                                // Celsius Button
                                Rectangle {
                                    width: 60; height: 34
                                    radius: 17
                                    color: ThemeController.tempUnit === 0 ? (theme.isDark ? "#4CAF50" : "#4CAF50") : "transparent"
                                    
                                    Behavior on color { ColorAnimation { duration: 200 } }
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "°C"
                                        color: ThemeController.tempUnit === 0 ? "white" : theme.textSecondary
                                        font.bold: true
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: ThemeController.setTempUnit(0)
                                    }
                                }
                                
                                // Fahrenheit Button
                                Rectangle {
                                    width: 60; height: 34
                                    radius: 17
                                    color: ThemeController.tempUnit === 1 ? (theme.isDark ? "#FF5722" : "#FF5722") : "transparent"
                                    
                                    Behavior on color { ColorAnimation { duration: 200 } }
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "°F"
                                        color: ThemeController.tempUnit === 1 ? "white" : theme.textSecondary
                                        font.bold: true
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: ThemeController.setTempUnit(1)
                                    }
                                }
                            }
                        }
                    }
                }

                // ══════════════════════════════════════════════════════════════
                // SECTION 3: LANGUAGE SETTINGS CARD
                // ══════════════════════════════════════════════════════════════
                Rectangle {
                    id: languageCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: languageContent.implicitHeight + 56
                    color: theme ? Qt.rgba(theme.surface.r, theme.surface.g, theme.surface.b, 0.9) : "#1a1a2e"
                    radius: 20
                    border.width: 1
                    border.color: theme && theme.isDark ? Qt.rgba(255,255,255,0.08) : Qt.rgba(0,0,0,0.08)
                    
                    // Subtle glow effect
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: 21
                        color: "transparent"
                        border.width: 2
                        border.color: Qt.rgba(0/255, 120/255, 212/255, 0.2)
                        z: -1
                    }
                    
                    ColumnLayout {
                        id: languageContent
                        width: parent.width - 56
                        anchors.centerIn: parent
                        spacing: 0
                        
                        // Header Row with premium dropdown
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 16
                            
                            Item {
                                width: 44; height: 44
                                
                        Canvas {
                            anchors.fill: parent
                            contextType: "2d"
                            
                            // Helper to draw rounded rect
                            function roundRect(ctx, x, y, w, h, r) {
                                ctx.beginPath();
                                ctx.moveTo(x + r, y);
                                ctx.lineTo(x + w - r, y);
                                ctx.quadraticCurveTo(x + w, y, x + w, y + r);
                                ctx.lineTo(x + w, y + h - r);
                                ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
                                ctx.lineTo(x + r, y + h);
                                ctx.quadraticCurveTo(x, y + h, x, y + h - r);
                                ctx.lineTo(x, y + r);
                                ctx.quadraticCurveTo(x, y, x + r, y);
                                ctx.closePath();
                            }

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                
                                // Layout
                                var W = width;
                                var H = height;
                                var cardSize = W * 0.55;
                                
                                // Font setup
                                ctx.font = "bold " + (cardSize * 0.7) + "px sans-serif";
                                ctx.textAlign = "center";
                                ctx.textBaseline = "middle";
                                
                                // 1. Back Card (Secondary language)
                                var backX = W * 0.4;
                                var backY = H * 0.15;
                                
                                ctx.fillStyle = theme && theme.isDark ? "#3a3a3a" : "#e0e0e0";
                                roundRect(ctx, backX, backY, cardSize, cardSize, 6);
                                ctx.fill();
                                
                                ctx.fillStyle = theme && theme.isDark ? "#ffffff" : "#444";
                                ctx.fillText("文", backX + cardSize/2, backY + cardSize/2 + 2); // 'Wen' (Language)
                                
                                // 2. Front Card (Primary Language)
                                var frontX = W * 0.05;
                                var frontY = H * 0.35;
                                
                                // Border for separation
                                ctx.lineWidth = 3;
                                ctx.strokeStyle = theme ? theme.surface : "#1a1a2e";
                                roundRect(ctx, frontX, frontY, cardSize, cardSize, 6);
                                ctx.stroke();
                                
                                // Fill
                                ctx.fillStyle = "#0078d4"; // Azure Blue
                                roundRect(ctx, frontX, frontY, cardSize, cardSize, 6);
                                ctx.fill();
                                
                                ctx.fillStyle = "#ffffff";
                                ctx.fillText("A", frontX + cardSize/2, frontY + cardSize/2);
                            }
                        }
                            }
                            
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                
                                Text {
                                    text: qsTr("LANGUAGE")
                                    color: "#0078d4"
                                    font.bold: true
                                    font.pixelSize: 14
                                    font.letterSpacing: 2
                                }
                                Text {
                                    text: qsTr("Select your preferred language")
                                    color: theme ? theme.textTertiary : "#888"
                                    font.pixelSize: 12
                                    opacity: 0.8
                                }
                            }
                            
                            // Spacer to push button to right
                            Item { Layout.fillWidth: true }
                            
                            // Premium Language Selector Button
                            Rectangle {
                                id: langSelector
                                Layout.preferredWidth: 200
                                Layout.preferredHeight: 44
                                radius: 12
                                color: langSelectorArea.containsMouse ? (theme && theme.isDark ? "#2a2a3e" : "#e8f4fc") : (theme && theme.isDark ? "#1e1e2e" : "#f0f7fc")
                                border.width: langSelectorArea.containsMouse ? 2 : 1
                                border.color: langSelectorArea.containsMouse ? "#0078d4" : Qt.rgba(0/255, 120/255, 212/255, 0.3)
                                
                                Behavior on color { ColorAnimation { duration: 200 } }
                                Behavior on border.color { ColorAnimation { duration: 200 } }
                                
                                MouseArea {
                                    id: langSelectorArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: langPopup.visible ? langPopup.close() : langPopup.open()
                                }
                                
                                Row {
                                    anchors.centerIn: parent
                                    anchors.leftMargin: 16
                                    anchors.rightMargin: 16
                                    spacing: 10
                                    
                                    // Locale Badge
                                    Rectangle {
                                        width: 34; height: 24
                                        radius: 4
                                        color: theme.isDark ? "#333" : "#ccc"
                                        border.width: 1
                                        border.color: theme.isDark ? "#555" : "#bbb"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            anchors.centerIn: parent
                                            text: LanguageController.currentLanguage.substring(0,2).toUpperCase()
                                            font.pixelSize: 11
                                            font.bold: true
                                            color: theme.textSecondary
                                        }
                                    }
                                    
                                    Text {
                                        text: LanguageController.getNativeName(LanguageController.currentLanguage)
                                        color: theme ? theme.textPrimary : "#fff"
                                        font.pixelSize: 14
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    Text {
                                        text: langPopup.visible ? "▲" : "▼"
                                        color: "#0078d4"
                                        font.pixelSize: 10
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Behavior on text { PropertyAnimation { duration: 150 } }
                                    }
                                }
                                
                                // Premium Dropdown Popup (Right-Aligned Dropdown)
                                Popup {
                                    id: langPopup
                                    y: parent.height + 8
                                    x: parent.width - width // Right-align with the button
                                    width: 300
                                    // Robust height calculation
                                    height: Math.min(LanguageController.availableLanguages.length * 62 + 24, 400)
                                    padding: 0
                                    z: 1000 // Ensure top-most stacking
                                    modal: false // Allow interaction with background
                                    dim: false
                                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                                    
                                    enter: Transition {
                                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
                                        NumberAnimation { property: "y"; from: langPopup.y + 10; to: langPopup.y; duration: 200; easing.type: Easing.OutBack }
                                    }
                                    exit: Transition {
                                        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
                                    }
                                    
                                    Overlay.modal: Rectangle {
                                        color: Qt.rgba(0, 0, 0, 0.2)
                                    }
                                    
                                    background: Rectangle {
                                        // Match Main Window Background (or Surface)
                                        // Assuming theme.background is the main bg, theme.surface is cards.
                                        // User complained about "purple" vs "settings background".
                                        // We'll use a safe dark/light color that matches the theme.
                                        color: theme.isDark ? "#121212" : "#ffffff" 
                                        radius: 12
                                        border.width: 1
                                        border.color: theme.isDark ? "#333" : "#ddd"
                                        
                                        // Drop shadow effect
                                        layer.enabled: true
                                        layer.effect: ShaderEffect {
                                            property var src: parent
                                        }
                                    }
                                    
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 8
                                        
                                        Text {
                                            text: qsTr("Choose Language")
                                            color: theme.textPrimary
                                            font.bold: true
                                            font.pixelSize: 14
                                            Layout.margins: 4
                                        }
                                        
                                        ListView {
                                            id: langListView
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            model: LanguageController.availableLanguages
                                            clip: true
                                            spacing: 8 // Consistent spacing
                                            
                                            delegate: Item {
                                                width: langListView.width
                                                height: 62

                                                Rectangle {
                                                    id: langDelegate
                                                    width: parent.width - 24
                                                    height: 54
                                                    anchors.centerIn: parent
                                                    radius: 10
                                                    color: langItemArea.containsMouse ? (theme.isDark ? "#2a2a3e" : "#f0f7fc") : (theme.isDark ? "#1e1e1e" : "#f9f9f9") // Distinct from bg
                                                    border.width: 1
                                                    border.color: langItemArea.containsMouse ? "#0078d4" : (theme.isDark ? "#333" : "#e0e0e0")
                                                    
                                                    Behavior on color { ColorAnimation { duration: 150 } }
                                                    Behavior on border.color { ColorAnimation { duration: 150 } }
                                                    
                                                    MouseArea {
                                                        id: langItemArea
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            if (modelData && modelData.code) {
                                                                LanguageController.setLanguage(modelData.code)
                                                                langPopup.close()
                                                            }
                                                        }
                                                    }
                                                    
                                                    RowLayout {
                                                        anchors.fill: parent
                                                        anchors.margins: 12
                                                        spacing: 16
                                                        
                                                        // Locale ID Badge
                                                        Rectangle {
                                                            width: 32; height: 22; radius: 4
                                                            color: theme.isDark ? "#333" : "#e5e5e5"
                                                            border.width: 1
                                                            border.color: theme.isDark ? "#444" : "#ddd"
                                                            Text { 
                                                                anchors.centerIn: parent
                                                                text: (modelData && modelData.code) ? modelData.code.substring(0,2).toUpperCase() : ""
                                                                font.pixelSize: 10; font.bold: true; color: theme.textSecondary 
                                                            }
                                                        }
                                                        
                                                        // Language Names (Centered)
                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            spacing: 0
                                                            
                                                            // Center Alignment Helper
                                                            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                                                            Text {
                                                                Layout.alignment: Qt.AlignHCenter
                                                                text: (modelData && modelData.nativeName) ? modelData.nativeName : ""
                                                                color: theme.textPrimary
                                                                font.pixelSize: 14; font.bold: true
                                                                horizontalAlignment: Text.AlignHCenter
                                                            }
                                                            Text {
                                                                Layout.alignment: Qt.AlignHCenter
                                                                text: (modelData && modelData.displayName) ? modelData.displayName : ""
                                                                color: theme.textTertiary
                                                                font.pixelSize: 11
                                                                visible: text !== parent.children[0].text // Hide if same
                                                                horizontalAlignment: Text.AlignHCenter
                                                            }
                                                        }
                                                        
                                                        // Checkmark (Fixed Space for Symmetry)
                                                        Item {
                                                            width: 24; height: 24
                                                            Text {
                                                                anchors.centerIn: parent
                                                                text: "✓"
                                                                color: "#0078d4"
                                                                opacity: LanguageController.currentLanguage === (modelData ? modelData.code : "") ? 1 : 0
                                                                font.bold: true; font.pixelSize: 18
                                                                Behavior on opacity { NumberAnimation { duration: 200 } }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } // End LanguageCard

                // ══════════════════════════════════════════════════════════════
                // SECTION 3: AUTHOR & ATTRIBUTIONS - Premium Redesign
                // ══════════════════════════════════════════════════════════════
                
                // Clipboard Helper (Hidden)
                TextEdit { id: clipboardHelper; visible: false }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: authorContent.implicitHeight + 40
                    color: theme ? Qt.rgba(theme.surface.r, theme.surface.g, theme.surface.b, 0.4) : "#101015"
                    radius: 16
                    border.width: 1
                    border.color: theme && theme.isDark ? Qt.rgba(255,255,255,0.06) : Qt.rgba(0,0,0,0.06)
                    
                    // Subtle glow effect
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: 17
                        color: "transparent"
                        border.width: 2
                        border.color: Qt.rgba(155/255, 89/255, 182/255, 0.2) // Purple glow for Author
                        z: -1
                    }
                    


                    ColumnLayout {
                        id: authorContent
                        width: parent.width - 48
                        anchors.centerIn: parent
                        spacing: 20
                        
                        // ══════════════════════════════════════════════════════════════
                        // SECTION 4: AUTHOR PROFILE
                        // ══════════════════════════════════════════════════════════════
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: authorLayout.implicitHeight + 48
                            color: theme ? Qt.rgba(theme.surface.r, theme.surface.g, theme.surface.b, 0.9) : "#1a1a2e"
                            radius: 20
                            border.width: 1
                            border.color: theme && theme.isDark ? Qt.rgba(255,255,255,0.08) : Qt.rgba(0,0,0,0.08)

                            // Halo Glow Effect
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -1
                                radius: 21
                                color: "transparent"
                                border.width: 2
                                border.color: Qt.rgba(156/255, 39/255, 176/255, 0.25) // Purple Glow
                                z: -1
                            }

                            ColumnLayout {
                                id: authorLayout
                                width: parent.width - 48
                                anchors.centerIn: parent
                                spacing: 20

                                // --- Section Header ---
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 20
                                    
                                    // Header Icon (Restored Code + Person Stacked Card)
                                    Rectangle {
                                        width: 48; height: 48
                                        color: "transparent"
                                        
                                        // 1. Back Card (Code Symbols)
                                        Rectangle {
                                            width: 26; height: 26
                                            x: 20; y: 8 // Offset
                                            radius: 6
                                            color: theme.isDark ? "#3a3a3a" : "#e0e0e0"
                                            
                                            Text {
                                                anchors.centerIn: parent
                                                text: "< >"
                                                color: theme.isDark ? "#888" : "#666"
                                                font.pixelSize: 10
                                                font.bold: true
                                                font.family: "Consolas"
                                            }
                                        }
                                        
                                        // 2. Front Card (Person Icon)
                                        Rectangle {
                                            width: 26; height: 26
                                            x: 2; y: 16 // Offset
                                            radius: 6
                                            color: "#9b59b6" // Purple Theme
                                            border.width: 2
                                            border.color: theme.surface // Match background for cutoff effect
                                            
                                            Shape {
                                                anchors.centerIn: parent
                                                width: 14; height: 14
                                                ShapePath {
                                                    strokeWidth: 0
                                                    fillColor: "white"
                                                    // Person Icon Path
                                                    PathSvg { path: "M7 7c1.29 0 2.33-1.04 2.33-2.33S8.29 2.33 7 2.33 4.67 3.38 4.67 4.67 5.71 7 7 7zm0 1.17c-1.56 0-4.67.78-4.67 2.33v1.17h9.33v-1.17c0-1.56-3.11-2.33-4.66-2.33z" }
                                                }
                                            }
                                        }
                                    }
                                    
                                    ColumnLayout {
                                        spacing: 2
                                        Text {
                                            text: qsTr("Author")
                                            color: theme.textPrimary
                                            font.bold: true
                                            font.pixelSize: 16
                                        }
                                        Text {
                                            text: qsTr("About the Developer")
                                            color: theme.textSecondary
                                            font.pixelSize: 13
                                        }
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                // --- The Final Hybrid Card (Nested) ---
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 240
                                    Layout.alignment: Qt.AlignHCenter

                                    MouseArea {
                                        id: hybridMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        preventStealing: true
                                    }

                                    // 3D Card Content
                                    Item {
                                        id: hybridCard
                                        width: parent.width // Fill the container width
                                        height: 200
                                        anchors.centerIn: parent

                                        transform: [
                                            Rotation {
                                                origin.x: hybridCard.width / 2
                                                origin.y: hybridCard.height / 2
                                                axis { x: 1; y: 0; z: 0 }
                                                angle: hybridMouse.containsMouse ? -(hybridMouse.mouseY - hybridCard.height/2) / 12 : 0
                                                Behavior on angle { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                                            },
                                            Rotation {
                                                origin.x: hybridCard.width / 2
                                                origin.y: hybridCard.height / 2
                                                axis { x: 0; y: 1; z: 0 }
                                                angle: hybridMouse.containsMouse ? (hybridMouse.mouseX - hybridCard.width/2) / 12 : 0
                                                Behavior on angle { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                                            }
                                        ]

                                        // 1. Spinning RGB Border
                                        Rectangle {
                                            anchors.fill: parent
                                            anchors.margins: -2
                                            radius: 18
                                            color: "transparent"
                                            
                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: parent.width * 1.5; height: parent.width * 1.5
                                                gradient: Gradient {
                                                    GradientStop { position: 0.0; color: "#ff0000" }
                                                    GradientStop { position: 0.33; color: "#00ff00" }
                                                    GradientStop { position: 0.66; color: "#0000ff" }
                                                    GradientStop { position: 1.0; color: "#ff0000" }
                                                }
                                                NumberAnimation on rotation {
                                                    from: 0; to: 360; duration: 4000; loops: Animation.Infinite; running: true
                                                }
                                            }
                                            layer.enabled: true
                                            layer.effect: ShaderEffect { property var src: parent }
                                        }

                                        // 2. Main Card Body
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 16
                                            color: theme.isDark ? "#121212" : "#ffffff"
                                            border.width: 1
                                            border.color: theme.isDark ? "#333" : "#ddd"

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 20
                                                spacing: 20

                                                // Hexagon Avatar
                                                Item {
                                                    Layout.preferredWidth: 100
                                                    Layout.preferredHeight: 115
                                                    
                                                    // Glow
                                                    Shape {
                                                        anchors.centerIn: parent
                                                        width: 104; height: 119
                                                        opacity: 0.5
                                                        ShapePath {
                                                            strokeWidth: 4; strokeColor: "#00ffff"; fillColor: "transparent"
                                                            startX: 52; startY: 0
                                                            PathLine { x: 104; y: 29.75 }
                                                            PathLine { x: 104; y: 89.25 }
                                                            PathLine { x: 52; y: 119 }
                                                            PathLine { x: 0; y: 89.25 }
                                                            PathLine { x: 0; y: 29.75 }
                                                            PathLine { x: 52; y: 0 }
                                                        }
                                                        SequentialAnimation on opacity {
                                                            loops: Animation.Infinite
                                                            NumberAnimation { to: 0.8; duration: 1500; easing.type: Easing.InOutQuad }
                                                            NumberAnimation { to: 0.4; duration: 1500; easing.type: Easing.InOutQuad }
                                                        }
                                                    }

                                                    // Hexagon
                                                    Shape {
                                                        anchors.centerIn: parent
                                                        width: 100; height: 115
                                                        ShapePath {
                                                            strokeWidth: 2; strokeColor: "#00ffff"; fillColor: "#2a2a2a"
                                                            startX: 50; startY: 0
                                                            PathLine { x: 100; y: 28.75 }
                                                            PathLine { x: 100; y: 86.25 }
                                                            PathLine { x: 50; y: 115 }
                                                            PathLine { x: 0; y: 86.25 }
                                                            PathLine { x: 0; y: 28.75 }
                                                            PathLine { x: 50; y: 0 }
                                                        }
                                                    }
                                                    
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "KR"
                                                        color: "#00ffff"
                                                        font.pixelSize: 32
                                                        font.bold: true
                                                        font.family: "Consolas"
                                                    }
                                                }

                                                // Info Column
                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 4
                                                    
                                                    Text {
                                                        text: qsTr("AUTHOR")
                                                        color: "#9b59b6"
                                                        font.bold: true
                                                        font.pixelSize: 10
                                                        font.letterSpacing: 2
                                                    }
                                                    
                                                    // Typing Name
                                                    Text {
                                                        id: hybridNameText
                                                        property string fullText: qsTr("Karthigaiselvam R")
                                                        property int currentIndex: 0
                                                        text: fullText.substring(0, currentIndex) + (cursorBlink2.visible ? "|" : "")
                                                        color: theme.textPrimary
                                                        font.pixelSize: 22
                                                        font.bold: true
                                                        font.family: "Consolas"
                                                        
                                                        Timer {
                                                            id: typeTimer
                                                            interval: 100; running: true; repeat: true
                                                            onTriggered: {
                                                                if (hybridNameText.currentIndex < hybridNameText.fullText.length) {
                                                                    hybridNameText.currentIndex++
                                                                } else {
                                                                    waitTimer.start(); running = false
                                                                }
                                                            }
                                                        }
                                                        Timer {
                                                            id: waitTimer
                                                            interval: 2000
                                                            onTriggered: { hybridNameText.currentIndex = 0; typeTimer.start() }
                                                        }
                                                        Timer {
                                                            id: cursorBlink2; interval: 500; running: true; repeat: true
                                                            property bool visible: true; onTriggered: visible = !visible
                                                        }
                                                    }
                                                    
                                                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider }

                                                    Text {
                                                        text: qsTr("Software Developer")
                                                        color: theme.textSecondary
                                                        font.pixelSize: 13
                                                        font.family: "Segoe UI"
                                                    }
                                                    
                                                    RowLayout {
                                                        spacing: 6
                                                        Rectangle { width: 8; height: 8; radius: 4; color: "#2ecc71" }
                                                        Text {
                                                            text: qsTr("Cyber Security Enthusiast")
                                                            color: "#2ecc71"
                                                            font.pixelSize: 12
                                                            font.bold: true
                                                            font.family: "Segoe UI"
                                                        }
                                                    }
                                                    
                                                    // New Quote Section
                                                    Text {
                                                        Layout.topMargin: 8
                                                        text: qsTr("\"Security is not a product, but a process.\"")
                                                        color: theme.textDisabled
                                                        font.italic: true
                                                        font.pixelSize: 11
                                                        font.family: "Segoe UI"
                                                        wrapMode: Text.WordWrap
                                                        Layout.fillWidth: true
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.divider; opacity: 0.3 }

                        // --- Social Buttons (Centered Row) ---
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            
                            // Spacer to center align the group
                            Item { Layout.fillWidth: true }
                            
                            // 1. LinkedIn
                            SocialBtn {
                                label: qsTr("LinkedIn")
                                btnColor: "#0077b5"
                                hoverColor: "#006097"
                                // Standard 24px LinkedIn Path
                                iconPath: "M19 3a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h14m-.5 15.5v-5.3a3.26 3.26 0 0 0-3.26-3.26c-.85 0-1.84.52-2.32 1.3v-1.11h-2.79v8.37h2.79v-4.93c0-.77.62-1.4 1.39-1.4a1.4 1.4 0 0 1 1.4 1.4v4.93h2.79M6.88 8.56a1.68 1.68 0 0 0 1.68-1.68c0-.93-.75-1.69-1.68-1.69a1.69 1.69 0 0 0-1.69 1.69c0 .93.76 1.68 1.69 1.68m1.39 9.94v-8.37H5.5v8.37h2.77z"
                                onClicked: Qt.openUrlExternally("https://www.linkedin.com/in/karthigaiselvam-r-7b9197258/")
                            }

                            // 2. GitHub
                            SocialBtn {
                                label: qsTr("GitHub")
                                btnColor: theme && theme.isDark ? "#333" : "#24292e"
                                hoverColor: "#2f363d"
                                // Standard 24px GitHub Path
                                iconPath: "M12 2A10 10 0 0 0 2 12c0 4.42 2.87 8.17 6.84 9.5c.5.08.66-.23.66-.5v-1.69c-2.77.6-3.36-1.34-3.36-1.34c-.46-1.16-1.11-1.47-1.11-1.47c-.91-.62.07-.6.07-.6c1 .07 1.53 1.03 1.53 1.03c.87 1.52 2.34 1.07 2.91.83c.09-.65.35-1.09.63-1.34c-2.22-.25-4.55-1.11-4.55-4.92c0-1.11.38-2 1.03-2.71c-.1-.25-.45-1.29.1-2.64c0 0 .84-.27 2.75 1.02c.79-.22 1.65-.33 2.5-.33c.85 0 1.71.11 2.5.33c1.91-1.29 2.75-1.02 2.75-1.02c.55 1.35.2 2.39.1 2.64c.65.71 1.03 1.6 1.03 2.71c0 3.82-2.34 4.66-4.57 4.91c.36.31.69.92.69 1.85V21c0 .27.16.59.67.5C19.14 20.16 22 16.42 22 12A10 10 0 0 0 12 2z"
                                onClicked: Qt.openUrlExternally("https://github.com/Karthigaiselvam-R-official/AsusTufController_Windows")
                            }

                            // 3. Email
                            SocialBtn {
                                label: qsTr("Email")
                                btnColor: "#d44638"
                                hoverColor: "#e0584b"
                                // Standard 24px Mail Path
                                iconPath: "M20 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 4l-8 5-8-5V6l8 5 8-5v2z"
                                onClicked: {
                                    clipboardHelper.text = "karthigaiselvamr.cs2022@gmail.com"
                                    clipboardHelper.selectAll(); clipboardHelper.copy()
                                    toast.show(qsTr("Email Copied"))
                                }
                            }
                            
                            // Spacer to center align the group
                            Item { Layout.fillWidth: true }
                        } // End RowLayout
                        
                        // Version Footer
                        Text {
                            Layout.fillWidth: true
                            text: "v1.0.0 • AsusTufFanControl"
                            color: theme.textDisabled
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignRight
                        }
                    } // End ColumnLayout (Author Content)
                } // End Rectangle (Author Section)
            } // End ColumnLayout (Main)
        } // End centerWrapper
    } // End Flickable

} // End Root Item
