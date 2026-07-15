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
        visible: tabBar.currentIndex === 0

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            Text {
                text: qsTr("My Home")
                font.pixelSize: 32
                font.weight: Font.Black
                color: "#1A237E"
            }
            Text {
                text: qsTr("System Online • %1 devices").arg(_lightCount + _rollerCount + _vacuumCount)
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
                            text: qsTr("LIGHTING")
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
                                                implicitWidth: 44; implicitHeight: 44
                                                contentItem: Text {
                                                    text: parent.text
                                                    font.pixelSize: 14
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                    color: "#7F8C8D"
                                                }
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
                                     handle: Rectangle {
                                         x: brightnessSlider.leftPadding + brightnessSlider.visualPosition * (brightnessSlider.availableWidth - width)
                                         y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                                         implicitWidth: 32; implicitHeight: 32
                                         radius: 16
                                         color: brightnessSlider.pressed ? "#1A237E" : "#2196F3"
                                         border.color: "white"
                                         border.width: 3
                                     }
                                     background: Rectangle {
                                         x: brightnessSlider.leftPadding
                                         y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                                         implicitWidth: 200; implicitHeight: 8
                                         width: brightnessSlider.availableWidth
                                         height: implicitHeight
                                         radius: 4
                                         color: "#E0E0E0"
                                         Rectangle {
                                             width: brightnessSlider.visualPosition * parent.width
                                             height: parent.height
                                             color: "#2196F3"
                                             radius: 4
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
                            text: qsTr("BLINDS & COMFORT")
                            font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
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
                                 implicitWidth: 24; implicitHeight: 24
                                 radius: 12
                                 color: masterRollerSlider.pressed ? "#1A237E" : "#2196F3"
                                 border.color: "white"
                                 border.width: 2
                             }
                             background: Rectangle {
                                 x: masterRollerSlider.leftPadding
                                 y: masterRollerSlider.topPadding + masterRollerSlider.availableHeight / 2 - height / 2
                                 implicitWidth: 100; implicitHeight: 6
                                 width: masterRollerSlider.availableWidth
                                 height: implicitHeight
                                 radius: 3
                                 color: "#E0E0E0"
                                 Rectangle {
                                     width: masterRollerSlider.visualPosition * parent.width
                                     height: parent.height
                                     color: "#2196F3"
                                     radius: 3
                                 }
                             }
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
                                            implicitWidth: 44; implicitHeight: 44
                                            contentItem: Text {
                                                text: parent.text
                                                font.pixelSize: 14
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                color: "#7F8C8D"
                                            }
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
                                     enabled: !model.isMoving
                                     onPressedChanged: {
                                         if (!pressed) {
                                             sensorBridge.publishCommand(model.topic, Math.round(value * 100).toString())
                                         }
                                     }
                                     handle: Rectangle {
                                         x: rollerSlider.leftPadding + rollerSlider.visualPosition * (rollerSlider.availableWidth - width)
                                         y: rollerSlider.topPadding + rollerSlider.availableHeight / 2 - height / 2
                                         implicitWidth: 32; implicitHeight: 32
                                         radius: 16
                                         color: rollerSlider.pressed ? "#1A237E" : "#2196F3"
                                         border.color: "white"
                                         border.width: 3
                                     }
                                     background: Rectangle {
                                         x: rollerSlider.leftPadding
                                         y: rollerSlider.topPadding + rollerSlider.availableHeight / 2 - height / 2
                                         implicitWidth: 200; implicitHeight: 8
                                         width: rollerSlider.availableWidth
                                         height: implicitHeight
                                         radius: 4
                                         color: "#E0E0E0"
                                         Rectangle {
                                             width: rollerSlider.visualPosition * parent.width
                                             height: parent.height
                                             color: "#2196F3"
                                             radius: 4
                                         }
                                     }
                                 }
                                
                                Button {
                                    visible: model.supportsStop && model.isMoving
                                    text: qsTr("🛑 STOP")
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
                            text: qsTr("VACUUMS & CLEANING")
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
                                             implicitWidth: 44; implicitHeight: 44
                                             contentItem: Text {
                                                 text: parent.text
                                                 font.pixelSize: 14
                                                 horizontalAlignment: Text.AlignHCenter
                                                 verticalAlignment: Text.AlignVCenter
                                                 color: "#7F8C8D"
                                             }
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
                                                 text: qsTr("Status: %1").arg(model.vacuumState ?? qsTr("unknown"))
                                                 font.pixelSize: 12
                                                 color: (model.vacuumState === "cleaning") ? "#4CAF50" : "#7F8C8D"
                                                 font.weight: Font.DemiBold
                                            }
                                            Text {
                                                 text: "• " + (model.fanSpeed ?? qsTr("Standard"))
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
                                         Text { text: qsTr("Battery"); font.pixelSize: 9; color: "#BDC3C7" }
                                    }
                                }
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    
                                    Button {
                                        Layout.fillWidth: true
                                         text: qsTr("▶ START")
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
                                         text: qsTr("⏸ PAUSE")
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
                                         text: qsTr("🏠 DOCK")
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
                        text: qsTr("ADMINISTRATION")
                        font.pixelSize: 12; font.weight: Font.Bold; color: "#9E9E9E"
                    }

                    Flow {
                        width: parent.width
                        spacing: 10
                        
                        Button {
                            text: "+ " + qsTr("Bath Light")
                            onClicked: sensorBridge.addDevice("Light", "BathroomLight", "home/light/bathroom/1")
                        }
                        Button {
                            text: "+ " + qsTr("Bed. Blind")
                            onClicked: sensorBridge.addDevice("Roller", "BedroomRoller", "home/roller/bedroom/1")
                        }
                        Button {
                            text: "+ " + qsTr("Living Blind")
                            onClicked: sensorBridge.addDevice("Roller", "StudioRoller", "home/roller/studio/1")
                        }
                        Button {
                            text: "+ " + qsTr("Vacuum")
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
                        text: activeVacuumDevice ? activeVacuumDevice.deviceId : qsTr("Vacuum Cleaner")
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
                        text: qsTr("Map not available\n(Make sure you have Xiaomi Cloud Map Extractor in HA)")
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
                        Text { text: qsTr("STATUS"); font.pixelSize: 9; color: "#9E9E9E"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
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
                        Text { text: qsTr("BATTERY"); font.pixelSize: 9; color: "#9E9E9E"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
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
                Text { text: qsTr("SUCTION POWER"); font.pixelSize: 10; font.weight: Font.Bold; color: "#9E9E9E" }
                
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
                Text { text: qsTr("MAINTENANCE & ACTIONS"); font.pixelSize: 10; font.weight: Font.Bold; color: "#9E9E9E" }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("🗺️ SCAN HOUSE")
                        onClicked: {
                            if (activeVacuumDevice) {
                                sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_start_mapping")
                            }
                        }
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("🗑️ EMPTY DUSTBIN")
                        onClicked: {
                            if (activeVacuumDevice) {
                                sensorBridge.publishCommand(activeVacuumDevice.topic, "SEND_COMMAND:app_empty_dustbin")
                            }
                        }
                    }

                    Button {
                        text: qsTr("📍 LOCATE")
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

    // PESTAÑA 1: ESCENAS INTELIGENTES
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        visible: tabBar.currentIndex === 1
        
        Item { Layout.fillHeight: true }
        Text { text: "🎬"; font.pixelSize: 64; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Smart Scenes"); font.pixelSize: 24; font.weight: Font.Bold; color: "#1A237E"; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Automate your home with a single touch."); font.pixelSize: 14; color: "#7F8C8D"; Layout.alignment: Qt.AlignHCenter }
        Item { Layout.fillHeight: true }
    }

    // PESTAÑA 2: CONFIGURACIÓN
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        visible: tabBar.currentIndex === 2

        Text { text: qsTr("Network Settings"); font.pixelSize: 28; font.weight: Font.Black; color: "#1A237E" }
        Text { text: qsTr("Link the mobile application with your Home Assistant server."); font.pixelSize: 13; color: "#7F8C8D"; Layout.fillWidth: true; wrapMode: Text.WordWrap }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Home Assistant WebSocket URL:"); font.weight: Font.Bold; color: "#34495E" }
            TextField {
                id: haUrlInput
                text: sensorBridge.getSavedHaUrl()
                placeholderText: "ws://192.168.178.20:8123/api/websocket"
                Layout.fillWidth: true; color: "#2C3E50"; font.pixelSize: 14
                background: Rectangle { implicitHeight: 44; radius: 6; border.color: haUrlInput.activeFocus ? "#1A237E" : "#BDC3C7"; border.width: haUrlInput.activeFocus ? 2 : 1 }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Long-Lived Access Token (LLAT):"); font.weight: Font.Bold; color: "#34495E" }
            TextField {
                id: haTokenInput
                text: sensorBridge.getSavedHaToken()
                placeholderText: qsTr("HA Token")
                Layout.fillWidth: true; color: "#2C3E50"; font.pixelSize: 14
                echoMode: TextInput.PasswordEchoOnEdit
                background: Rectangle { implicitHeight: 44; radius: 6; border.color: haTokenInput.activeFocus ? "#1A237E" : "#BDC3C7"; border.width: haTokenInput.activeFocus ? 2 : 1 }
            }
        }

        Button {
            text: qsTr("Save and Connect")
            Layout.fillWidth: true; implicitHeight: 48
            contentItem: Text { text: parent.text; font.weight: Font.Bold; font.pixelSize: 16; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.pressed ? "#0D47A1" : "#1A237E"; radius: 8 }
            onClicked: {
                sensorBridge.saveHaCredentials(haUrlInput.text, haTokenInput.text)
                statusText.text = qsTr("Saved. Attempting to connect...")
                statusTimer.start()
            }
        }

        Text { id: statusText; text: ""; font.pixelSize: 12; color: "#4CAF50"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
        Timer { id: statusTimer; interval: 3000; onTriggered: statusText.text = "" }

        // Sección de Idioma
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Label { text: qsTr("Language:"); font.weight: Font.Bold; color: "#34495E" }
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

        Text { id: languageStatusText; text: ""; font.pixelSize: 12; color: "#E74C3C"; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
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
            color: "white"
            radius: 12
            border.color: "#E0E6ED"
        }

        ColumnLayout {
            spacing: 12
            anchors.fill: parent
            anchors.margins: 10
            Label {
                text: qsTr("Enter the new visual name:")
                font.weight: Font.Bold
                color: "#2C3E50"
            }
            TextField {
                id: aliasInput
                focus: true
                placeholderText: qsTr("Alias")
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
        TabButton { text: qsTr("Dashboard") }
        TabButton { text: qsTr("Scenes") }
        TabButton { text: qsTr("Config") }
    }
}
