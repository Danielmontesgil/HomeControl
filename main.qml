import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SensorsApp 1.0

ApplicationWindow {
    visible: true
    width: 450
    height: 750
    title: "HomeControl Smart"

    background: Rectangle { color: "#F5F7FA" }

    // Propiedades internas para forzar la reactividad de los conteos
    property int _lightCount: sensorBridge.getCountByType(0)
    property int _rollerCount: sensorBridge.getCountByType(1)
    property int _vacuumCount: sensorBridge.getCountByType(2)
    property var activeVacuumDevice: null
    property string deviceToRename: ""

    Connections {
        target: sensorBridge
        function onCountChanged() {
            // Cuando el C++ avisa, actualizamos nuestras propiedades locales
            // Esto forzará a los Switches y Sliders maestros a re-evaluarse
            _lightCount = sensorBridge.getCountByType(0)
            _rollerCount = sensorBridge.getCountByType(1)
            _vacuumCount = sensorBridge.getCountByType(2)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            Text {
                text: "Mi Hogar"
                font.pixelSize: 32
                font.weight: Font.Black
                color: "#1A237E"
            }
            Text {
                text: "Sistema Online • " + (_lightCount + _rollerCount + _vacuumCount) + " dispositivos"
                font.pixelSize: 13
                color: "#4CAF50"
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: -1

            Column {
                width: parent.width
                spacing: 25

                // --- SECCIÓN: ILUMINACIÓN ---
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _lightCount > 0
                    
                    RowLayout {
                        width: parent.width
                        Label {
                            text: "ILUMINACIÓN"
                            font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
                            Layout.fillWidth: true
                        }
                        Switch {
                            id: masterLightSwitch
                            visible: _lightCount > 1
                            scale: 0.8
                            checked: sensorBridge.getCountByType(0) > 0
                            
                            indicator: Rectangle {
                                implicitWidth: 48
                                implicitHeight: 26
                                radius: 13
                                color: masterLightSwitch.checked ? "#4CAF50" : "#BDC3C7"
                                Rectangle {
                                    x: masterLightSwitch.checked ? parent.width - width - 2 : 2
                                    y: 2
                                    width: 22; height: 22; radius: 11; color: "white"
                                    Behavior on x { NumberAnimation { duration: 150 } }
                                }
                            }
                            onClicked: sensorBridge.setAllDevicesState(0, checked ? "ON" : "OFF")
                        }
                    }

                    Repeater {
                        model: sensorBridge.devices
                        delegate: Rectangle {
                            id: lightDeviceDelegate
                            width: parent.width 
                            height: model.deviceType === 0 ? (model.supportsColor ? 170 : (model.deviceValue !== undefined ? 130 : 90)) : 0
                            visible: model.deviceType === 0
                            color: "white"
                            radius: 12
                            border.color: "#E0E6ED"
                            
                            readonly property string deviceTopic: model.topic
                            readonly property string currentDeviceColor: model.deviceColor ?? ""
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 15
                                spacing: 10
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 15
                                    Rectangle {
                                        width: 45; height: 45; radius: 8
                                        color: model.isOn ? (model.supportsColor ? model.deviceColor : "#FFF9C4") : "#E0E0E0"
                                        Text { anchors.centerIn: parent; text: "💡"; font.pixelSize: 20; opacity: model.isOn ? 1.0 : 0.4 }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 5
                                            Text { text: model.deviceId; font.weight: Font.Bold; font.pixelSize: 16; color: "#2C3E50"; Layout.fillWidth: true; elide: Text.ElideRight }
                                            Button {
                                                text: "✏️"
                                                flat: true
                                                implicitWidth: 24; implicitHeight: 24
                                                scale: 0.8
                                                onClicked: {
                                                    deviceToRename = model.topic
                                                    aliasInput.text = model.deviceId
                                                    renameDialog.open()
                                                }
                                            }
                                        }
                                        Text { text: model.topic; font.pixelSize: 11; color: "#95A5A6" }
                                    }
                                    Switch {
                                        id: deviceSwitch
                                        checked: model.isOn
                                        
                                        indicator: Rectangle {
                                            implicitWidth: 40
                                            implicitHeight: 22
                                            radius: 11
                                            color: deviceSwitch.checked ? "#4CAF50" : "#BDC3C7"
                                            Rectangle {
                                                x: deviceSwitch.checked ? parent.width - width - 2 : 2
                                                y: 2
                                                width: 18; height: 18; radius: 9; color: "white"
                                                Behavior on x { NumberAnimation { duration: 150 } }
                                            }
                                        }
                                        onClicked: sensorBridge.publishCommand(model.topic, checked ? "ON" : "OFF")
                                    }
                                }

                                Slider {
                                    id: brightnessSlider
                                    Layout.fillWidth: true
                                    value: model.deviceValue ?? 0.0
                                    visible: model.deviceValue !== undefined
                                    onPressedChanged: {
                                        if (!pressed) {
                                            sensorBridge.publishCommand(model.topic, "BRIGHTNESS:" + Math.round(value * 100))
                                        }
                                    }
                                }

                                Row {
                                    spacing: 10
                                    visible: model.supportsColor === true
                                    z: 1 // Force this overlay on top of any Slider bounds to intercept clicks
                                    
                                    Repeater {
                                        model: ["#FF3B30", "#34C759", "#007AFF", "#FFCC00", "#AF52DE", "#FFFFFF"]
                                        delegate: Rectangle {
                                            width: 18; height: 18; radius: 9
                                            color: modelData
                                            border.color: lightDeviceDelegate.currentDeviceColor === modelData ? "#2C3E50" : "#BDC3C7"
                                            border.width: lightDeviceDelegate.currentDeviceColor === modelData ? 2 : 1
                                            
                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    console.log("QML: Clicking color " + modelData + " for device " + lightDeviceDelegate.deviceTopic);
                                                    sensorBridge.publishCommand(lightDeviceDelegate.deviceTopic, "COLOR:" + modelData);
                                                }
                                            }
                                        }
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
                            text: "PERSIANAS Y CONFORT"
                            font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
                            Layout.fillWidth: true
                        }
                        Slider {
                            visible: _rollerCount > 1
                            Layout.preferredWidth: 100
                            onMoved: sensorBridge.setAllDevicesState(1, Math.round(value * 100).toString())
                        }
                    }

                    Repeater {
                        model: sensorBridge.devices
                        delegate: Rectangle {
                            width: parent.width
                            height: model.deviceType === 1 ? 120 : 0
                            visible: model.deviceType === 1
                            color: "white"
                            radius: 12
                            border.color: "#E0E6ED"
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 15
                                spacing: 10
                                RowLayout {
                                    Layout.fillWidth: true
                                    Rectangle { 
                                        width: 35; height: 35; radius: 6; 
                                        color: model.isMoving ? "#BBDEFB" : "#E0E0E0"
                                        Text { anchors.centerIn: parent; text: "🪟"; font.pixelSize: 16; opacity: model.isMoving ? 1.0 : 0.5 } 
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 5
                                        Text { text: model.deviceId; font.weight: Font.Bold; font.pixelSize: 16; Layout.fillWidth: true; elide: Text.ElideRight; opacity: model.isMoving ? 1.0 : 0.7 }
                                        Button {
                                            text: "✏️"
                                            flat: true
                                            implicitWidth: 24; implicitHeight: 24
                                            scale: 0.8
                                            onClicked: {
                                                deviceToRename = model.topic
                                                aliasInput.text = model.deviceId
                                                renameDialog.open()
                                            }
                                        }
                                    }
                                    Text { text: Math.round((model.deviceValue ?? 0) * 100) + "%"; color: model.isMoving ? "#2196F3" : "#757575"; font.weight: Font.Black }
                                }
                                Slider {
                                    id: rollerSlider
                                    Layout.fillWidth: true
                                    value: model.deviceValue ?? 0.0
                                    // Bloqueamos la interacción directa mientras se mueve
                                    enabled: !model.isMoving
                                    
                                    onPressedChanged: {
                                        if (!pressed) { // Al soltar
                                            sensorBridge.publishCommand(model.topic, Math.round(value * 100).toString())
                                        }
                                    }
                                }
                                
                                Button {
                                    visible: model.supportsStop && model.isMoving
                                    text: "🛑 DETENER"
                                    flat: true
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#F44336"
                                        font.weight: Font.Bold
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    onClicked: sensorBridge.stopDevice(model.topic)
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
                            text: "ASPIRADORES Y LIMPIEZA"
                            font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: sensorBridge.devices
                        delegate: Rectangle {
                            width: parent.width
                            height: model.deviceType === 2 ? 140 : 0
                            visible: model.deviceType === 2
                            color: "white"
                            radius: 12
                            border.color: "#E0E6ED"

                            MouseArea {
                                anchors.fill: parent
                                z: 1
                                onClicked: {
                                    activeVacuumDevice = model
                                    vacuumDetailsPopup.open()
                                }
                            }
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 15
                                z: 2
                                spacing: 10
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 15
                                    
                                    Rectangle {
                                        width: 45; height: 45; radius: 22.5
                                        color: (model.vacuumState === "cleaning") ? "#E8F5E9" : "#F5F5F5"
                                        Text { 
                                            anchors.centerIn: parent
                                            text: "🧹"
                                            font.pixelSize: 22
                                            
                                            RotationAnimation on rotation {
                                                running: model.vacuumState === "cleaning"
                                                loops: Animation.Infinite
                                                from: 0; to: 360
                                                duration: 3000
                                            }
                                        }
                                    }
                                    
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 5
                                            Text { text: model.deviceId ?? ""; font.weight: Font.Bold; font.pixelSize: 16; color: "#2C3E50"; Layout.fillWidth: true; elide: Text.ElideRight }
                                            Button {
                                                text: "✏️"
                                                flat: true
                                                implicitWidth: 24; implicitHeight: 24
                                                scale: 0.8
                                                onClicked: {
                                                    deviceToRename = model.topic
                                                    aliasInput.text = model.deviceId
                                                    renameDialog.open()
                                                }
                                            }
                                        }
                                        RowLayout {
                                            spacing: 5
                                            Text { 
                                                text: "Estado: " + (model.vacuumState ?? "desconocido")
                                                font.pixelSize: 12
                                                color: (model.vacuumState === "cleaning") ? "#4CAF50" : "#7F8C8D"
                                                font.weight: Font.DemiBold
                                            }
                                            Text {
                                                text: "• " + (model.fanSpeed ?? "Standard")
                                                font.pixelSize: 11
                                                color: "#95A5A6"
                                            }
                                        }
                                    }
                                    
                                    ColumnLayout {
                                        Layout.alignment: Qt.AlignVCenter
                                        spacing: 2
                                        Text {
                                            text: (model.batteryLevel !== undefined ? model.batteryLevel : 100) + "%"
                                            font.pixelSize: 14
                                            font.weight: Font.Black
                                            color: (model.batteryLevel !== undefined && model.batteryLevel > 20) ? "#4CAF50" : "#F44336"
                                        }
                                        Text { text: "Batería"; font.pixelSize: 9; color: "#BDC3C7" }
                                    }
                                }
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    
                                    Button {
                                        Layout.fillWidth: true
                                        text: "▶ EMPEZAR"
                                        enabled: model.vacuumState !== "cleaning"
                                        onClicked: sensorBridge.publishCommand(model.topic, "START")
                                        contentItem: Text {
                                            text: parent.text
                                            color: parent.enabled ? "#4CAF50" : "#BDC3C7"
                                            font.weight: Font.Bold
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                    
                                    Button {
                                        Layout.fillWidth: true
                                        text: "⏸ PAUSAR"
                                        enabled: model.vacuumState === "cleaning"
                                        onClicked: sensorBridge.publishCommand(model.topic, "PAUSE")
                                        contentItem: Text {
                                            text: parent.text
                                            color: parent.enabled ? "#FFA000" : "#BDC3C7"
                                            font.weight: Font.Bold
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                    
                                    Button {
                                        Layout.fillWidth: true
                                        text: "🏠 CARGAR"
                                        enabled: model.vacuumState !== "docked" && model.vacuumState !== "returning"
                                        onClicked: sensorBridge.publishCommand(model.topic, "RETURN")
                                        contentItem: Text {
                                            text: parent.text
                                            color: parent.enabled ? "#2196F3" : "#BDC3C7"
                                            font.weight: Font.Bold
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }

                                    Button {
                                        text: "📍"
                                        onClicked: sensorBridge.publishCommand(model.topic, "LOCATE")
                                    }
                                }
                            }
                        }
                    }
                }

                // --- SECCIÓN: ADMINISTRACIÓN ---
                Column {
                    width: parent.width
                    spacing: 15
                    topPadding: 20

                    Label {
                        text: "ADMINISTRACIÓN"
                        font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
                    }

                    Flow {
                        width: parent.width
                        spacing: 10
                        
                        Button {
                            text: "+ Luz Baño"
                            onClicked: sensorBridge.addDevice("Light", "BathroomLight", "home/light/bathroom/1")
                        }
                        Button {
                            text: "+ Persiana Hab."
                            onClicked: sensorBridge.addDevice("Roller", "BedroomRoller", "home/roller/bedroom/1")
                        }
                        Button {
                            text: "+ Persiana Salón"
                            onClicked: sensorBridge.addDevice("Roller", "StudioRoller", "home/roller/studio/1")
                        }
                        Button {
                            text: "+ Aspiradora"
                            onClicked: sensorBridge.addDevice("Vacuum", "Xiaomi Vacuum 20X+", "vacuum.xiaomi_vacuum")
                        }
                    }
                }
            }
        }
    }

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
            color: "white"
            radius: 20
            border.color: "#E0E6ED"
            border.width: 1
        }
        
        Timer {
            id: mapRefreshTimer
            interval: 5000
            running: vacuumDetailsPopup.opened
            repeat: true
            onTriggered: {
                mapImage.refresh()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            RowLayout {
                Layout.fillWidth: true
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: activeVacuumDevice ? activeVacuumDevice.deviceId : "Aspirador"
                        font.weight: Font.Black
                        font.pixelSize: 22
                        color: "#1A237E"
                    }
                    Text {
                        text: activeVacuumDevice ? activeVacuumDevice.topic : ""
                        font.pixelSize: 11
                        color: "#7F8C8D"
                    }
                }

                Button {
                    text: "❌"
                    flat: true
                    onClicked: vacuumDetailsPopup.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#F8F9FA"
                radius: 12
                clip: true
                border.color: "#E0E6ED"

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
                        text: "Mapa no disponible\n(Asegúrate de tener Xiaomi Cloud Map Extractor en HA)"
                        font.pixelSize: 12
                        color: "#95A5A6"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 15

                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    radius: 8
                    color: "#F5F7FA"
                    ColumnLayout {
                        anchors.centerIn: parent
                        Text { text: "ESTADO"; font.pixelSize: 9; color: "#9E9E9E"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                        Text { 
                            text: activeVacuumDevice ? activeVacuumDevice.vacuumState : "-"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: activeVacuumDevice && activeVacuumDevice.vacuumState === "cleaning" ? "#4CAF50" : "#2C3E50"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    radius: 8
                    color: "#F5F7FA"
                    ColumnLayout {
                        anchors.centerIn: parent
                        Text { text: "BATERÍA"; font.pixelSize: 9; color: "#9E9E9E"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                        Text { 
                            text: (activeVacuumDevice && activeVacuumDevice.batteryLevel !== undefined ? activeVacuumDevice.batteryLevel : 100) + "%"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: activeVacuumDevice && activeVacuumDevice.batteryLevel > 20 ? "#4CAF50" : "#F44336"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Text { text: "POTENCIA DE SUCCIÓN"; font.pixelSize: 10; font.weight: Font.Bold; color: "#9E9E9E" }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Repeater {
                        model: ["Silent", "Standard", "Medium", "Turbo"]
                        delegate: Button {
                            Layout.fillWidth: true
                            text: modelData
                            
                            readonly property bool isCurrent: activeVacuumDevice && activeVacuumDevice.fanSpeed === modelData
                            
                            background: Rectangle {
                                color: parent.isCurrent ? "#1A237E" : "#ECEFF1"
                                radius: 6
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: parent.isCurrent ? "white" : "#37474F"
                                font.weight: parent.isCurrent ? Font.Bold : Font.Normal
                                horizontalAlignment: Text.AlignHCenter
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
                Layout.fillWidth: true
                spacing: 5
                Text { text: "MANTENIMIENTO Y ACCIONES"; font.pixelSize: 10; font.weight: Font.Bold; color: "#9E9E9E" }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Button {
                        Layout.fillWidth: true
                        text: "🗺️ ESCANEAR CASA"
                        onClicked: {
                            if (activeVacuumDevice) {
                                sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_start_mapping")
                            }
                        }
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "🗑️ VACIAR DEPÓSITO"
                        onClicked: {
                            if (activeVacuumDevice) {
                                sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_empty_dustbin")
                            }
                        }
                    }

                    Button {
                        text: "📍 LOCALIZAR"
                        onClicked: {
                            if (activeVacuumDevice) {
                                sensorBridge.publishCommand(activeVacuumDevice.topic, "LOCATE")
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: renameDialog
        title: "Renombrar dispositivo"
        standardButtons: Dialog.Save | Dialog.Cancel
        anchors.centerIn: parent
        modal: true
        
        onOpened: aliasInput.forceActiveFocus()
        
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#E0E6ED"
        }

        ColumnLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 10
            Label {
                text: "Introduce el nuevo nombre visual:"
                font.weight: Font.Bold
                color: "#2C3E50"
            }
            TextField {
                id: aliasInput
                focus: true
                placeholderText: "Alias"
                color: "#2C3E50"
                placeholderTextColor: "#95A5A6"
                Layout.fillWidth: true
                background: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 40
                    radius: 6
                    border.color: aliasInput.activeFocus ? "#1A237E" : "#BDC3C7"
                }
            }
        }
        
        onAccepted: {
            sensorBridge.renameDevice(deviceToRename, aliasInput.text)
        }
    }

    footer: TabBar {
        id: tabBar
        currentIndex: 0
        TabButton { text: "Panel" }
        TabButton { text: "Escenas" }
        TabButton { text: "Config" }
    }
}
