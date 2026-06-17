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

    Connections {
        target: sensorBridge
        function onCountChanged() {
            // Cuando el C++ avisa, actualizamos nuestras propiedades locales
            // Esto forzará a los Switches y Sliders maestros a re-evaluarse
            _lightCount = sensorBridge.getCountByType(0)
            _rollerCount = sensorBridge.getCountByType(1)
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
                text: "Sistema Online • " + (_lightCount + _rollerCount) + " dispositivos"
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
                            visible: _lightCount > 1
                            scale: 0.8
                            onClicked: sensorBridge.setAllDevicesState(0, checked ? "ON" : "OFF")
                        }
                    }

                    Repeater {
                        model: sensorBridge.devices
                        delegate: Rectangle {
                            width: parent.width 
                            height: model.deviceType === 0 ? 90 : 0
                            visible: model.deviceType === 0
                            color: "white"
                            radius: 12
                            border.color: "#E0E6ED"
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 15
                                spacing: 15
                                Rectangle {
                                    width: 45; height: 45; radius: 8
                                    color: model.deviceValue > 0 ? "#FFF9C4" : "#F5F5F5"
                                    Text { anchors.centerIn: parent; text: "💡"; font.pixelSize: 20 }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Text { text: model.deviceId; font.weight: Font.Bold; font.pixelSize: 16; color: "#2C3E50"; Layout.fillWidth: true; elide: Text.ElideRight }
                                    Text { text: model.topic; font.pixelSize: 11; color: "#95A5A6" }
                                }
                                Switch {
                                    checked: model.deviceValue > 0
                                    onClicked: sensorBridge.publishCommand(model.topic, checked ? "ON" : "OFF")
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
                                    Rectangle { width: 35; height: 35; radius: 6; color: "#E3F2FD"; Text { anchors.centerIn: parent; text: "🪟"; font.pixelSize: 16 } }
                                    Text { text: model.deviceId; font.weight: Font.Bold; font.pixelSize: 16; Layout.fillWidth: true; elide: Text.ElideRight }
                                    Text { text: Math.round(model.deviceValue * 100) + "%"; color: "#2196F3"; font.weight: Font.Black }
                                }
                                Slider {
                                    Layout.fillWidth: true
                                    value: model.deviceValue
                                    onMoved: sensorBridge.publishCommand(model.topic, Math.round(value * 100).toString())
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
                    }
                }
            }
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
