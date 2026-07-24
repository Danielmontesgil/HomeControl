import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SensorsApp 1.0

ApplicationWindow {
    id: window
    visible: true
    width: (Qt.platform.os === "android" || Qt.platform.os === "ios") ? Screen.width : 450
    height: (Qt.platform.os === "android" || Qt.platform.os === "ios") ? Screen.height : 750
    title: qsTr("HomeControl Smart")

    // --- VARIABLES DE ESTILO Y TEMA OSCURO ---
    readonly property color colorBg: "#0A0E1A"
    readonly property color colorCardBg: "#151B2E"
    readonly property color colorCardBgActive: "#1E2640"
    readonly property color colorBorder: "#1F293D"
    readonly property color colorBorderActive: "#3B82F6"
    readonly property color colorTextPrimary: "#FFFFFF"
    readonly property color colorTextSecondary: "#94A3B8"
    readonly property color colorAccent: "#3B82F6"
    readonly property color colorSuccess: "#10B981"
    readonly property color colorWarning: "#F59E0B"
    readonly property color colorDanger: "#EF4444"

    background: Rectangle { color: colorBg }

    // Propiedades internas para forzar la reactividad de los conteos
    property int _lightCount: sensorBridge.getCountByType(0)
    property int _rollerCount: sensorBridge.getCountByType(1)
    property int _vacuumCount: sensorBridge.getCountByType(2)
    property var activeVacuumDevice: null
    property var activeControlDevice: null
    property string deviceToRename: ""
    property bool showFpsOverlay: false
    property int currentFps: 60

    FrameAnimation {
        id: globalFpsCounter
        running: window.showFpsOverlay
        
        property int frameCount: 0
        property real elapsedTime: 0.0
        
        onTriggered: {
            frameCount++
            elapsedTime += frameTime
            if (elapsedTime >= 0.5) {
                window.currentFps = Math.round(frameCount / elapsedTime)
                frameCount = 0
                elapsedTime = 0.0
            }
        }
    }

    Connections {
        target: sensorBridge
        function onCountChanged() {
            _lightCount = sensorBridge.getCountByType(0)
            _rollerCount = sensorBridge.getCountByType(1)
            _vacuumCount = sensorBridge.getCountByType(2)
        }
    }

    // --- PESTAÑA 0: DASHBOARD PRINCIPAL ---
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 18
        visible: tabBar.currentIndex === 0

        // Cabecera Principal
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Text {
                    text: qsTr("My Home")
                    font.pixelSize: 32
                    font.weight: Font.Black
                    color: colorTextPrimary
                }
                Text {
                    text: qsTr("System Online • %1 devices").arg(_lightCount + _rollerCount + _vacuumCount)
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: colorSuccess
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                spacing: 8
                
                // Píldora flotante de FPS
                Rectangle {
                    visible: window.showFpsOverlay
                    height: 28
                    width: 58
                    radius: 14
                    color: colorCardBg
                    border.color: colorBorder
                    border.width: 1
                    
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: window.currentFps >= 55 ? colorSuccess : (window.currentFps >= 30 ? colorWarning : colorDanger)
                        }
                        Text {
                            text: window.currentFps + " FPS"
                            color: colorTextPrimary
                            font.pixelSize: 10; font.weight: Font.Bold
                        }
                    }
                }

                NetworkStatusIndicator {
                    id: netIndicator
                    onClicked: diagnosticsSheet.open()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: -1

            Column {
                width: parent.width
                spacing: 24

                // --- SECCIÓN: ILUMINACIÓN ---
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _lightCount > 0
                    
                    RowLayout {
                        width: parent.width
                        Label {
                            text: qsTr("LIGHTING")
                            font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                            Layout.fillWidth: true
                        }
                        Switch {
                            id: masterLightSwitch
                            visible: _lightCount > 1
                            scale: 0.85
                            checked: sensorBridge.getCountByType(0) > 0
                            
                            indicator: Rectangle {
                                implicitWidth: 44
                                implicitHeight: 24
                                radius: 12
                                color: masterLightSwitch.checked ? colorSuccess : "#1F293D"
                                Rectangle {
                                    x: masterLightSwitch.checked ? parent.width - width - 2 : 2
                                    y: 2
                                    width: 20; height: 20; radius: 10; color: "white"
                                    Behavior on x { NumberAnimation { duration: 150 } }
                                }
                            }
                            onClicked: sensorBridge.setAllDevicesState(0, checked ? "ON" : "OFF")
                        }
                    }

                    Grid {
                        id: lightsGrid
                        width: parent.width
                        columns: parent.width > 500 ? 3 : 2
                        spacing: 12

                        Repeater {
                            model: sensorBridge.devices
                            delegate: Rectangle {
                                visible: model.deviceType === 0
                                width: visible ? (lightsGrid.width - (lightsGrid.spacing * (lightsGrid.columns - 1))) / lightsGrid.columns : 0
                                height: visible ? 110 : 0
                                color: model.isOn ? colorCardBgActive : colorCardBg
                                radius: 16
                                border.color: model.isOn ? colorBorderActive : colorBorder
                                border.width: 1
                                opacity: model.available ? 1.0 : 0.4
                                Behavior on opacity { NumberAnimation { duration: 150 } }

                                scale: cardMouseArea.pressed ? 0.95 : 1.0
                                Behavior on scale { NumberAnimation { duration: 80 } }
                                Behavior on color { ColorAnimation { duration: 150 } }

                                // Efecto de resplandor para tarjetas encendidas
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: -4
                                    radius: 20
                                    color: "transparent"
                                    border.color: colorBorderActive
                                    border.width: 1
                                    opacity: model.isOn ? 0.25 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Rectangle {
                                            width: 32; height: 32; radius: 16
                                            color: model.isOn ? (model.supportsColor ? model.deviceColor : "#FFE082") : "#1F2538"
                                            Text { 
                                                anchors.centerIn: parent; 
                                                text: "💡"; 
                                                font.pixelSize: 15; 
                                                opacity: model.isOn ? 1.0 : 0.4 
                                            }
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: (model.isOn && model.deviceValue !== undefined) ? Math.round(model.deviceValue * 100) + "%" : ""
                                            color: colorTextSecondary
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                        }
                                    }

                                    Item { Layout.fillHeight: true }

                                    Text {
                                        text: model.deviceId
                                        color: colorTextPrimary
                                        font.weight: Font.Bold
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: !model.available ? qsTr("Unavailable") : (model.isOn ? qsTr("Active") : qsTr("Off"))
                                        color: !model.available ? colorDanger : (model.isOn ? colorSuccess : colorTextSecondary)
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                }

                                MouseArea {
                                    id: cardMouseArea
                                    anchors.fill: parent
                                    enabled: model.available
                                    onClicked: {
                                        sensorBridge.publishCommand(model.topic, model.isOn ? "OFF" : "ON")
                                    }
                                    onPressAndHold: {
                                        activeControlDevice = model
                                        lightControlPopup.open()
                                    }
                                }
                            }
                        }
                    }
                }

                // --- SECCIÓN: PERSIANAS ---
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _rollerCount > 0

                    RowLayout {
                        width: parent.width
                        Label {
                            text: qsTr("BLINDS & COMFORT")
                            font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                            Layout.fillWidth: true
                        }
                        Slider {
                            id: masterRollerSlider
                            visible: _rollerCount > 1
                            Layout.preferredWidth: 100
                            onMoved: sensorBridge.setAllDevicesState(1, Math.round(value * 100).toString())
                            handle: Rectangle {
                                x: masterRollerSlider.leftPadding + masterRollerSlider.visualPosition * (masterRollerSlider.availableWidth - width)
                                y: masterRollerSlider.topPadding + masterRollerSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18; implicitHeight: 18
                                radius: 9
                                color: masterRollerSlider.pressed ? colorAccent : "#FFFFFF"
                                border.color: colorAccent
                                border.width: 1
                            }
                            background: Rectangle {
                                x: masterRollerSlider.leftPadding
                                y: masterRollerSlider.topPadding + masterRollerSlider.availableHeight / 2 - height / 2
                                implicitWidth: 100; implicitHeight: 6
                                width: masterRollerSlider.availableWidth
                                height: implicitHeight
                                radius: 3
                                color: "#1F293D"
                                Rectangle {
                                    width: masterRollerSlider.visualPosition * parent.width
                                    height: parent.height
                                    color: colorAccent
                                    radius: 3
                                }
                            }
                        }
                    }

                    Grid {
                        id: rollersGrid
                        width: parent.width
                        columns: parent.width > 500 ? 3 : 2
                        spacing: 12

                        Repeater {
                            model: sensorBridge.devices
                            delegate: Rectangle {
                                visible: model.deviceType === 1
                                width: visible ? (rollersGrid.width - (rollersGrid.spacing * (rollersGrid.columns - 1))) / rollersGrid.columns : 0
                                height: visible ? 110 : 0
                                color: model.isMoving ? colorCardBgActive : colorCardBg
                                radius: 16
                                border.color: model.isMoving ? colorWarning : colorBorder
                                border.width: 1
                                opacity: model.available ? 1.0 : 0.4
                                Behavior on opacity { NumberAnimation { duration: 150 } }

                                scale: rollerMouseArea.pressed ? 0.95 : 1.0
                                Behavior on scale { NumberAnimation { duration: 80 } }
                                Behavior on color { ColorAnimation { duration: 150 } }

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: -4
                                    radius: 20
                                    color: "transparent"
                                    border.color: colorWarning
                                    border.width: 1
                                    opacity: model.isMoving ? 0.25 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Rectangle {
                                            width: 32; height: 32; radius: 16
                                            color: model.isMoving ? "#FFF3E0" : "#1F2538"
                                            Text { 
                                                anchors.centerIn: parent; 
                                                text: "🪟"; 
                                                font.pixelSize: 15; 
                                                opacity: model.isMoving ? 1.0 : 0.5 
                                            }
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: model.available ? Math.round((model.deviceValue ?? 0) * 100) + "%" : ""
                                            color: colorTextPrimary
                                            font.pixelSize: 12; font.weight: Font.Black
                                        }
                                    }

                                    Item { Layout.fillHeight: true }

                                    Text {
                                        text: model.deviceId
                                        color: colorTextPrimary
                                        font.weight: Font.Bold
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: !model.available ? qsTr("Unavailable") : (model.isMoving ? qsTr("Moving") : qsTr("Idle"))
                                        color: !model.available ? colorDanger : (model.isMoving ? colorWarning : colorTextSecondary)
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                }

                                MouseArea {
                                    id: rollerMouseArea
                                    anchors.fill: parent
                                    enabled: model.available
                                    onClicked: {
                                        activeControlDevice = model
                                        rollerControlPopup.open()
                                    }
                                    onPressAndHold: {
                                        activeControlDevice = model
                                        rollerControlPopup.open()
                                    }
                                }
                            }
                        }
                    }
                }

                // --- SECCIÓN: ASPIRADORES ---
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _vacuumCount > 0

                    RowLayout {
                        width: parent.width
                        Label {
                            text: qsTr("VACUUMS & CLEANING")
                            font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: sensorBridge.devices
                        delegate: Rectangle {
                            visible: model.deviceType === 2
                            width: visible ? parent.width : 0
                            height: visible ? 130 : 0
                            color: colorCardBg
                            radius: 16
                            border.color: colorBorder
                            border.width: 1
                            opacity: model.available ? 1.0 : 0.4
                            Behavior on opacity { NumberAnimation { duration: 150 } }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Rectangle {
                                        width: 38; height: 38; radius: 19
                                        color: (model.vacuumState === "cleaning") ? "#E8F5E9" : "#1F2538"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "🧹"
                                            font.pixelSize: 18
                                            RotationAnimation on rotation {
                                                running: model.vacuumState === "cleaning"
                                                loops: Animation.Infinite
                                                from: 0; to: 360; duration: 3000
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Layout.fillWidth: true
                                        Text { 
                                            text: model.deviceId ?? ""
                                            color: colorTextPrimary
                                            font.weight: Font.Bold; font.pixelSize: 15; elide: Text.ElideRight 
                                        }
                                        RowLayout {
                                            spacing: 5
                                            Text {
                                                text: !model.available ? qsTr("Status: Unavailable") : qsTr("Status: %1").arg(model.vacuumState ?? qsTr("unknown"))
                                                font.pixelSize: 12
                                                color: !model.available ? colorDanger : ((model.vacuumState === "cleaning") ? colorSuccess : colorTextSecondary)
                                                font.weight: Font.DemiBold
                                            }
                                            Text {
                                                text: "• " + (model.fanSpeed ?? qsTr("Standard"))
                                                font.pixelSize: 11
                                                color: colorTextSecondary
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Text {
                                            text: model.available ? ((model.batteryLevel !== undefined ? model.batteryLevel : 100) + "%") : ""
                                            font.pixelSize: 14; font.weight: Font.Black
                                            color: (model.batteryLevel !== undefined && model.batteryLevel > 20) ? colorSuccess : colorDanger
                                        }
                                        Text { text: qsTr("Battery"); font.pixelSize: 9; color: colorTextSecondary; Layout.alignment: Qt.AlignRight }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        text: qsTr("START")
                                        enabled: model.available && model.vacuumState !== "cleaning"
                                        padding: 0
                                        background: Rectangle { color: parent.enabled ? "#1F3A2B" : "#151B2E"; border.color: parent.enabled ? colorSuccess : colorBorder; radius: 8 }
                                        contentItem: Text { text: parent.text; color: parent.enabled ? colorSuccess : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: sensorBridge.publishCommand(model.topic, "START")
                                    }

                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        text: qsTr("PAUSE")
                                        enabled: model.available && model.vacuumState === "cleaning"
                                        padding: 0
                                        background: Rectangle { color: parent.enabled ? "#3D2B1F" : "#151B2E"; border.color: parent.enabled ? colorWarning : colorBorder; radius: 8 }
                                        contentItem: Text { text: parent.text; color: parent.enabled ? colorWarning : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: sensorBridge.publishCommand(model.topic, "PAUSE")
                                    }

                                    Button {
                                        Layout.fillWidth: true
                                        implicitHeight: 36
                                        text: qsTr("DOCK")
                                        enabled: model.available && model.vacuumState !== "docked" && model.vacuumState !== "returning"
                                        padding: 0
                                        background: Rectangle { color: parent.enabled ? "#1F2E3D" : "#151B2E"; border.color: parent.enabled ? colorAccent : colorBorder; radius: 8 }
                                        contentItem: Text { text: parent.text; color: parent.enabled ? colorAccent : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: sensorBridge.publishCommand(model.topic, "RETURN")
                                    }

                                    Button {
                                        implicitWidth: 36; implicitHeight: 36
                                        text: "🗺️"
                                        padding: 0
                                        background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                                        contentItem: Text { text: parent.text; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: {
                                            activeVacuumDevice = model
                                            vacuumDetailsPopup.open()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // --- SECCIÓN: ADMINISTRACIÓN ---
                Column {
                    width: parent.width
                    spacing: 12
                    topPadding: 10

                    Label {
                        text: qsTr("ADMINISTRATION")
                        font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                    }

                    Flow {
                        width: parent.width
                        spacing: 8
                        
                        Button {
                            text: "+ " + qsTr("Bath Light")
                            padding: 0
                            implicitHeight: 32; implicitWidth: 100
                            background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                            contentItem: Text { text: parent.text; color: colorTextPrimary; font.pixelSize: 12; font.weight: Font.Medium; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: sensorBridge.addDevice("Light", "BathroomLight", "home/light/bathroom/1")
                        }
                        Button {
                            text: "+ " + qsTr("Bed. Blind")
                            padding: 0
                            implicitHeight: 32; implicitWidth: 100
                            background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                            contentItem: Text { text: parent.text; color: colorTextPrimary; font.pixelSize: 12; font.weight: Font.Medium; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: sensorBridge.addDevice("Roller", "BedroomRoller", "home/roller/bedroom/1")
                        }
                        Button {
                            text: "+ " + qsTr("Living Blind")
                            padding: 0
                            implicitHeight: 32; implicitWidth: 100
                            background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                            contentItem: Text { text: parent.text; color: colorTextPrimary; font.pixelSize: 12; font.weight: Font.Medium; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: sensorBridge.addDevice("Roller", "StudioRoller", "home/roller/studio/1")
                        }
                        Button {
                            text: "+ " + qsTr("Vacuum")
                            padding: 0
                            implicitHeight: 32; implicitWidth: 100
                            background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                            contentItem: Text { text: parent.text; color: colorTextPrimary; font.pixelSize: 12; font.weight: Font.Medium; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: sensorBridge.addDevice("Vacuum", "Xiaomi Vacuum 20X+", "vacuum.xiaomi_vacuum")
                        }
                    }
                }
            }
        }
    }

    // --- POPUPS Y DETALLES CONTEXTUALES (BOTTOM SHEETS) ---

    // 1. Popup de Control de Luz (Brillo + Colores)
    Popup {
        id: lightControlPopup
        width: parent.width
        height: 330
        y: parent.height - height
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        enter: Transition {
            NumberAnimation { property: "y"; from: window.height; to: window.height - 330; duration: 250; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "y"; from: window.height - 330; to: window.height; duration: 200; easing.type: Easing.InCubic }
        }

        background: Rectangle {
            color: colorCardBg
            radius: 24
            Rectangle {
                width: parent.width; height: 24; y: parent.height - 24; color: colorCardBg
            }
            Rectangle {
                width: parent.width; height: 1; color: colorBorder
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            Rectangle {
                width: 36; height: 5; radius: 2.5; color: "#374151"
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: activeControlDevice ? activeControlDevice.deviceId : ""
                        color: colorTextPrimary
                        font.pixelSize: 18; font.weight: Font.Bold
                    }
                    Text {
                        text: activeControlDevice ? activeControlDevice.topic : ""
                        color: colorTextSecondary; font.pixelSize: 11
                    }
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "✏️"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        if (activeControlDevice) {
                            deviceToRename = activeControlDevice.topic
                            aliasInput.text = activeControlDevice.deviceId
                            renameDialog.open()
                        }
                    }
                }
                Button {
                    text: "X"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 18; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: lightControlPopup.close()
                }
            }

            // Slider de Brillo iOS-Style
            ColumnLayout {
                Layout.fillWidth: true
                visible: activeControlDevice && activeControlDevice.deviceValue !== undefined
                spacing: 6
                Label { text: qsTr("BRIGHTNESS"); font.pixelSize: 10; color: colorTextSecondary; font.weight: Font.Bold }
                Slider {
                    id: brightnessSlider
                    Layout.fillWidth: true
                    value: activeControlDevice ? activeControlDevice.deviceValue : 0.0
                    onPressedChanged: {
                        if (!pressed && activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, "BRIGHTNESS:" + Math.round(value * 100))
                        }
                    }
                    background: Rectangle {
                        implicitHeight: 24; radius: 12; color: "#1F293D"
                        Rectangle {
                            width: brightnessSlider.visualPosition * parent.width; height: parent.height
                            color: colorAccent; radius: 12
                        }
                    }
                    handle: Rectangle {
                        x: brightnessSlider.leftPadding + brightnessSlider.visualPosition * (brightnessSlider.availableWidth - width)
                        y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                        implicitWidth: 32; implicitHeight: 32; radius: 16; color: "#FFFFFF"; border.color: colorAccent; border.width: 2
                    }
                }
            }

            // Preset de Colores
            ColumnLayout {
                Layout.fillWidth: true
                visible: activeControlDevice && activeControlDevice.supportsColor
                spacing: 6
                Label { text: qsTr("COLOR PRESETS"); font.pixelSize: 10; color: colorTextSecondary; font.weight: Font.Bold }
                RowLayout {
                    spacing: 12
                    Repeater {
                        model: ["#FF3B30", "#34C759", "#007AFF", "#FFCC00", "#AF52DE", "#FFFFFF"]
                        delegate: Rectangle {
                            width: 36; height: 36; radius: 18
                            color: modelData
                            border.color: (activeControlDevice && activeControlDevice.deviceColor === modelData) ? colorTextPrimary : colorBorder
                            border.width: (activeControlDevice && activeControlDevice.deviceColor === modelData) ? 3 : 1
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (activeControlDevice) {
                                        sensorBridge.publishCommand(activeControlDevice.topic, "COLOR:" + modelData)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. Popup de Control de Persiana (Vertical Slider + STOP)
    Popup {
        id: rollerControlPopup
        width: parent.width
        height: 360
        y: parent.height - height
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        enter: Transition {
            NumberAnimation { property: "y"; from: window.height; to: window.height - 360; duration: 250; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "y"; from: window.height - 360; to: window.height; duration: 200; easing.type: Easing.InCubic }
        }

        background: Rectangle {
            color: colorCardBg
            radius: 24
            Rectangle {
                width: parent.width; height: 24; y: parent.height - 24; color: colorCardBg
            }
            Rectangle {
                width: parent.width; height: 1; color: colorBorder
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            Rectangle {
                width: 36; height: 5; radius: 2.5; color: "#374151"
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: activeControlDevice ? activeControlDevice.deviceId : ""
                        color: colorTextPrimary
                        font.pixelSize: 18; font.weight: Font.Bold
                    }
                    Text {
                        text: activeControlDevice ? activeControlDevice.topic : ""
                        color: colorTextSecondary; font.pixelSize: 11
                    }
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "✏️"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        if (activeControlDevice) {
                            deviceToRename = activeControlDevice.topic
                            aliasInput.text = activeControlDevice.deviceId
                            renameDialog.open()
                        }
                    }
                }
                Button {
                    text: "X"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 18; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: rollerControlPopup.close()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 30

                // Slider Vertical de persiana
                Slider {
                    id: rollerSlider
                    orientation: Qt.Vertical
                    Layout.preferredHeight: 160
                    Layout.preferredWidth: 40
                    Layout.alignment: Qt.AlignVCenter
                    value: activeControlDevice ? activeControlDevice.deviceValue : 0.0
                    onPressedChanged: {
                        if (!pressed && activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, Math.round(value * 100).toString())
                        }
                    }
                    background: Rectangle {
                        implicitWidth: 24; implicitHeight: 160; radius: 12; color: "#1F293D"
                        Rectangle {
                            y: parent.height - height
                            width: parent.width; height: rollerSlider.visualPosition * parent.height
                            color: colorAccent; radius: 12
                        }
                    }
                    handle: Rectangle {
                        x: rollerSlider.leftPadding + rollerSlider.availableWidth / 2 - width / 2
                        y: rollerSlider.topPadding + (1.0 - rollerSlider.visualPosition) * (rollerSlider.availableHeight - height)
                        implicitWidth: 32; implicitHeight: 32; radius: 16; color: "#FFFFFF"; border.color: colorAccent; border.width: 2
                    }
                }

                ColumnLayout {
                    spacing: 15
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Text {
                        text: activeControlDevice ? qsTr("Current Position: %1%").arg(Math.round((activeControlDevice.deviceValue ?? 0) * 100)) : ""
                        color: colorTextPrimary
                        font.pixelSize: 16; font.weight: Font.Bold
                    }

                    Button {
                        Layout.fillWidth: true
                        implicitHeight: 48
                        text: qsTr("🛑 STOP")
                        padding: 0
                        background: Rectangle { color: colorDanger; radius: 8 }
                        contentItem: Text { text: parent.text; font.weight: Font.Bold; font.pixelSize: 16; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (activeControlDevice) {
                                sensorBridge.stopDevice(activeControlDevice.topic)
                                rollerControlPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    // 3. Popup Detalles del Aspirador (Mapa + Controles)
    Popup {
        id: vacuumDetailsPopup
        width: parent.width * 0.95
        height: parent.height * 0.85
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: colorCardBg
            radius: 20
            border.color: colorBorder
            border.width: 1
        }
        
        Timer {
            id: mapRefreshTimer
            interval: 5000
            running: vacuumDetailsPopup.opened
            repeat: true
            onTriggered: mapImage.refresh()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: activeVacuumDevice ? activeVacuumDevice.deviceId : qsTr("Vacuum Cleaner")
                        font.weight: Font.Black; font.pixelSize: 20; color: colorTextPrimary
                    }
                    Text {
                        text: activeVacuumDevice ? activeVacuumDevice.topic : ""
                        font.pixelSize: 11; color: colorTextSecondary
                    }
                }
                Button {
                    text: "✏️"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        if (activeVacuumDevice) {
                            deviceToRename = activeVacuumDevice.topic
                            aliasInput.text = activeVacuumDevice.deviceId
                            renameDialog.open()
                        }
                    }
                }
                Button {
                    text: "X"
                    flat: true
                    padding: 0
                    implicitWidth: 32; implicitHeight: 32
                    contentItem: Text { text: parent.text; color: colorTextSecondary; font.pixelSize: 18; font.weight: Font.Bold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: vacuumDetailsPopup.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: colorBg
                radius: 12
                clip: true
                border.color: colorBorder

                Image {
                    id: mapImage
                    anchors.fill: parent
                    anchors.margins: 10
                    fillMode: Image.PreserveAspectFit
                    property string baseSource: activeVacuumDevice ? "image://hacamera/camera.xiaomi_cloud_map_extractor" : ""
                    source: baseSource
                    
                    function refresh() {
                        if (baseSource !== "") {
                            source = baseSource + "?t=" + Date.now()
                        }
                    }
                    onBaseSourceChanged: refresh()

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: mapImage.status === Image.Loading
                    }
                    
                    Text {
                        anchors.centerIn: parent
                        visible: mapImage.status === Image.Error || !activeVacuumDevice
                        text: qsTr("Map not available\n(Make sure you have Xiaomi Cloud Map Extractor in HA)")
                        font.pixelSize: 12
                        color: colorTextSecondary
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true; height: 50; radius: 8; color: "#1F293D"
                    ColumnLayout {
                        anchors.centerIn: parent
                        Text { text: qsTr("STATUS"); font.pixelSize: 8; color: colorTextSecondary; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                        Text { 
                            text: activeVacuumDevice ? activeVacuumDevice.vacuumState : "-"
                            font.pixelSize: 14; font.weight: Font.Bold
                            color: activeVacuumDevice && activeVacuumDevice.vacuumState === "cleaning" ? colorSuccess : colorTextPrimary
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 50; radius: 8; color: "#1F293D"
                    ColumnLayout {
                        anchors.centerIn: parent
                        Text { text: qsTr("BATTERY"); font.pixelSize: 8; color: colorTextSecondary; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                        Text { 
                            text: (activeVacuumDevice && activeVacuumDevice.batteryLevel !== undefined ? activeVacuumDevice.batteryLevel : 100) + "%"
                            font.pixelSize: 14; font.weight: Font.Bold
                            color: activeVacuumDevice && activeVacuumDevice.batteryLevel > 20 ? colorSuccess : colorDanger
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                Text { text: qsTr("SUCTION POWER"); font.pixelSize: 9; font.weight: Font.Bold; color: colorTextSecondary }
                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: ["Silent", "Standard", "Medium", "Turbo"]
                        delegate: Button {
                            Layout.fillWidth: true
                            text: modelData
                            padding: 0
                            readonly property bool isCurrent: activeVacuumDevice && activeVacuumDevice.fanSpeed === modelData
                            background: Rectangle {
                                color: parent.isCurrent ? colorAccent : "#1F293D"
                                radius: 6
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.isCurrent ? "white" : colorTextPrimary
                                font.weight: parent.isCurrent ? Font.Bold : Font.Normal; font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                if (activeVacuumDevice) {
                                    sensorBridge.publishCommand(activeVacuumDevice.topic, "FAN_SPEED:" + modelData)
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                Text { text: qsTr("MAINTENANCE & ACTIONS"); font.pixelSize: 9; font.weight: Font.Bold; color: colorTextSecondary }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("🗺️ SCAN HOUSE")
                        padding: 0
                        implicitHeight: 32
                        background: Rectangle { color: "#1F293D"; radius: 6 }
                        contentItem: Text { text: parent.text; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: activeVacuumDevice && sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_start_mapping")
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("🗑️ EMPTY")
                        padding: 0
                        implicitHeight: 32
                        background: Rectangle { color: "#1F293D"; radius: 6 }
                        contentItem: Text { text: parent.text; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: activeVacuumDevice && sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_empty_dustbin")
                    }
                    Button {
                        text: qsTr("📍 LOCATE")
                        padding: 0
                        implicitHeight: 32
                        implicitWidth: 80
                        background: Rectangle { color: "#1F293D"; radius: 6 }
                        contentItem: Text { text: parent.text; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: activeVacuumDevice && sensorBridge.publishCommand(activeVacuumDevice.topic, "LOCATE")
                    }
                }
            }
        }
    }

    // --- PESTAÑA 1: ESCENAS INTELIGENTES ---
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        visible: tabBar.currentIndex === 1
        
        Item { Layout.fillHeight: true }
        Text { text: "🎬"; font.pixelSize: 64; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Smart Scenes"); font.pixelSize: 24; font.weight: Font.Bold; color: colorTextPrimary; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Automate your home with a single touch."); font.pixelSize: 14; color: colorTextSecondary; Layout.alignment: Qt.AlignHCenter }
        Item { Layout.fillHeight: true }
    }

    // --- PESTAÑA 2: CONFIGURACIÓN ---
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        visible: tabBar.currentIndex === 2

        Text { text: qsTr("Network Settings"); font.pixelSize: 28; font.weight: Font.Black; color: colorTextPrimary }
        Text { text: qsTr("Link the mobile application with your Home Assistant server."); font.pixelSize: 13; color: colorTextSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Home Assistant WebSocket URL:"); font.weight: Font.Bold; color: colorTextPrimary }
            TextField {
                id: haUrlInput
                text: sensorBridge.getSavedHaUrl()
                placeholderText: "ws://192.168.178.20:8123/api/websocket"
                Layout.fillWidth: true; color: colorTextPrimary; font.pixelSize: 14
                placeholderTextColor: "#475569"
                background: Rectangle { implicitHeight: 44; color: colorCardBg; radius: 6; border.color: haUrlInput.activeFocus ? colorAccent : colorBorder; border.width: haUrlInput.activeFocus ? 2 : 1 }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Long-Lived Access Token (LLAT):"); font.weight: Font.Bold; color: colorTextPrimary }
            TextField {
                id: haTokenInput
                text: sensorBridge.getSavedHaToken()
                placeholderText: qsTr("HA Token")
                Layout.fillWidth: true; color: colorTextPrimary; font.pixelSize: 14
                placeholderTextColor: "#475569"
                echoMode: TextInput.PasswordEchoOnEdit
                background: Rectangle { implicitHeight: 44; color: colorCardBg; radius: 6; border.color: haTokenInput.activeFocus ? colorAccent : colorBorder; border.width: haTokenInput.activeFocus ? 2 : 1 }
            }
        }

        Button {
            text: qsTr("Save and Connect")
            Layout.fillWidth: true; implicitHeight: 48
            contentItem: Text { text: parent.text; font.weight: Font.Bold; font.pixelSize: 16; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.pressed ? "#1D4ED8" : colorAccent; radius: 8 }
            onClicked: {
                sensorBridge.saveHaCredentials(haUrlInput.text, haTokenInput.text)
                statusText.text = qsTr("Saved. Attempting to connect...")
                statusTimer.start()
            }
        }

        Text { id: statusText; text: ""; font.pixelSize: 12; color: colorSuccess; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
        Timer { id: statusTimer; interval: 3000; onTriggered: statusText.text = "" }

        // Sección de Idioma
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Language:"); font.weight: Font.Bold; color: colorTextPrimary }
            ComboBox {
                id: languageCombo
                Layout.fillWidth: true
                model: [
                    { text: qsTr("System Default"), value: "system" },
                    { text: "English", value: "en" },
                    { text: "Español", value: "es" },
                    { text: "Deutsch", value: "de" }
                ]
                textRole: "text"
                valueRole: "value"
                
                contentItem: Text {
                    text: languageCombo.displayText
                    color: colorTextPrimary
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }
                
                background: Rectangle {
                    implicitHeight: 40
                    color: colorCardBg
                    border.color: colorBorder
                    radius: 6
                }
                
                Component.onCompleted: {
                    var saved = sensorBridge.getSavedLanguage()
                    for (var i = 0; i < model.length; i++) {
                        if (model[i].value === saved) {
                            currentIndex = i
                            break
                        }
                    }
                }
                
                onActivated: {
                    var selectedValue = model[currentIndex].value
                    sensorBridge.saveLanguage(selectedValue)
                    languageStatusText.text = qsTr("Language changed. Please restart the app to apply.")
                }
            }
        }

        Text { id: languageStatusText; text: ""; font.pixelSize: 12; color: colorDanger; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
        Item { Layout.fillHeight: true }
    }

    Dialog {
        id: renameDialog
        title: qsTr("Rename device")
        standardButtons: Dialog.Save | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        
        onOpened: aliasInput.forceActiveFocus()
        
        background: Rectangle {
            color: colorCardBg
            radius: 12
            border.color: colorBorder
        }

        ColumnLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 10
            Label {
                text: qsTr("Enter the new visual name:")
                font.weight: Font.Bold
                color: colorTextPrimary
            }
            TextField {
                id: aliasInput
                focus: true
                placeholderText: qsTr("Alias")
                color: colorTextPrimary
                placeholderTextColor: "#475569"
                Layout.fillWidth: true
                background: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 40
                    color: colorBg
                    radius: 6
                    border.color: aliasInput.activeFocus ? colorAccent : colorBorder
                }
            }
        }
        
        onAccepted: {
            sensorBridge.renameDevice(deviceToRename, aliasInput.text)
        }
    }

    NetworkDiagnosticsSheet {
        id: diagnosticsSheet
        onOpenDevTools: {
            devToolsLoader.active = true
            devToolsLoader.item.open()
        }
    }

    Loader {
        id: devToolsLoader
        active: false
        source: sensorBridge.isDebugBuild ? "DevToolsConsole.qml" : ""
    }

    Shortcut {
        sequence: "Ctrl+Shift+D"
        enabled: sensorBridge.isDebugBuild
        onActivated: {
            devToolsLoader.active = true
            devToolsLoader.item.open()
        }
    }

    // Pie de Navegación Flotante Estilizado y Centrado
    footer: Rectangle {
        color: "transparent"
        height: 72
        
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 32
            height: 52
            radius: 26
            color: colorCardBg
            border.color: colorBorder
            border.width: 1
            
            Row {
                id: tabBar
                anchors.fill: parent
                property int currentIndex: 0

                Repeater {
                    model: [
                        { name: qsTr("Dashboard"), idx: 0 },
                        { name: qsTr("Scenes"), idx: 1 },
                        { name: qsTr("Config"), idx: 2 }
                    ]
                    delegate: Item {
                        width: tabBar.width / 3
                        height: tabBar.height

                        Text {
                            text: modelData.name
                            color: tabBar.currentIndex === modelData.idx ? colorAccent : colorTextSecondary
                            font.weight: Font.Bold; font.pixelSize: 12
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: tabBar.currentIndex = modelData.idx
                        }
                    }
                }
            }
        }
    }
}
