import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: devToolsPopup
    parent: Overlay.overlay
    width: Math.min(window.width - 20, 420)
    height: Math.min(window.height - 40, 680)
    modal: true
    focus: true
    anchors.centerIn: parent
    padding: 0

    background: Rectangle {
        color: "#0F172A" // Deep slate dark mode
        radius: 16
        border.color: "#334155"
        border.width: 1.5
    }

    contentItem: Rectangle {
        id: contentRect
        color: "#0F172A" // Deep slate dark mode
        radius: 16
        border.color: "#334155"
        border.width: 1.5



        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

        // Title and Close
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true
                Text {
                    text: "🛠️ DevTools Control Panel"
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: "#F1F5F9"
                }
                Text {
                    text: "Version: " + (sensorBridge.buildVersion || "Unknown") + " | Build: #" + (sensorBridge.buildNumber || "0")
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: "#818CF8"
                }
                Text {
                    text: "Compiled: " + (sensorBridge.buildTimestamp || "Unknown")
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: "#94A3B8"
                }
            }
            Button {
                text: "X"
                flat: true
                implicitWidth: 32; implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "#94A3B8"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: devToolsPopup.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#334155"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 20

                // SECTION 1: CHAOS ENGINEERING
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "CHAOS SIMULATION"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: "#818CF8" // Cool Indigo
                    }

                    // Force Disconnect Button
                    Button {
                        text: "Force Socket Disconnect"
                        Layout.fillWidth: true
                        implicitHeight: 38
                        contentItem: Text {
                            text: parent.text
                            font.weight: Font.Bold
                            font.pixelSize: 12
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.pressed ? "#991B1B" : "#DC2626"
                            radius: 6
                        }
                        onClicked: sensorBridge.forceDisconnect()
                    }

                    // Latency Slider
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Inject Delay (Latency):"; font.pixelSize: 13; color: "#E2E8F0"; Layout.fillWidth: true }
                            Text {
                                text: latencySlider.value === 0 ? "0 ms (None)" : Math.round(latencySlider.value) + " ms"
                                font.pixelSize: 13; font.weight: Font.Bold; color: "#818CF8"
                            }
                        }
                        Slider {
                            id: latencySlider
                            Layout.fillWidth: true
                            from: 0
                            to: 2000
                            stepSize: 250
                            value: 0
                            onMoved: sensorBridge.setSimulationLatency(value)
                        }
                    }

                    // Offline simulation switch
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Simulate Offline Mode"
                            font.pixelSize: 13
                            color: "#E2E8F0"
                            Layout.fillWidth: true
                        }
                        Switch {
                            id: offlineSwitch
                            checked: false
                            onCheckedChanged: sensorBridge.setSimulationOfflineMode(checked)
                        }
                    }

                    // Auth failure switch
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Simulate Auth Fail (Invalid Token)"
                            font.pixelSize: 13
                            color: "#E2E8F0"
                            Layout.fillWidth: true
                        }
                        Switch {
                            id: authFailSwitch
                            checked: false
                            onCheckedChanged: sensorBridge.setSimulationAuthFail(checked)
                        }
                    }

                    // Verbose Console Logging switch
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Log JSON Traffic to Console"
                            font.pixelSize: 13
                            color: "#E2E8F0"
                            Layout.fillWidth: true
                        }
                        Switch {
                            id: verboseLogSwitch
                            checked: sensorBridge.verboseLogging
                            onCheckedChanged: sensorBridge.verboseLogging = checked
                        }
                    }

                    // Floating FPS Overlay switch
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Show Floating FPS Overlay"
                            font.pixelSize: 13
                            color: "#E2E8F0"
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: window.showFpsOverlay
                            onCheckedChanged: window.showFpsOverlay = checked
                        }
                    }

                    // Seccion de Filtrado de Logs por Bits
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#1E293B"
                    }

                    Text {
                        text: "ACTIVE LOG LEVELS (BITMASK)"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: "#818CF8"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        function toggleBit(bit) {
                            var current = sensorBridge.logMask;
                            if ((current & bit) !== 0) {
                                sensorBridge.logMask = current & ~bit;
                            } else {
                                sensorBridge.logMask = current | bit;
                            }
                        }

                        CheckBox {
                            text: "INFO"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            checked: (sensorBridge.logMask & 1) !== 0
                            onClicked: parent.toggleBit(1)
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "#38BDF8" : "#94A3B8"
                                leftPadding: 24
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        CheckBox {
                            text: "DEBUG"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            checked: (sensorBridge.logMask & 2) !== 0
                            onClicked: parent.toggleBit(2)
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "#38BDF8" : "#94A3B8"
                                leftPadding: 24
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        CheckBox {
                            text: "WARN"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            checked: (sensorBridge.logMask & 4) !== 0
                            onClicked: parent.toggleBit(4)
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "#F59E0B" : "#94A3B8"
                                leftPadding: 24
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        CheckBox {
                            text: "ERROR"
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            checked: (sensorBridge.logMask & 8) !== 0
                            onClicked: parent.toggleBit(8)
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "#EF4444" : "#94A3B8"
                                leftPadding: 24
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#1E293B"
                }

                // SECTION 2: RAW DEVICE MODEL INSPECTOR
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "DEVICE MODEL DATA (" + sensorBridge.devices.rowCount() + " entities)"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: "#34D399" // Mint Green
                    }

                    Column {
                        width: parent.width
                        spacing: 8

                        Repeater {
                            model: sensorBridge.devices
                            delegate: Rectangle {
                                width: parent.width
                                height: 75
                                color: "#1E293B"
                                radius: 8
                                border.color: "#334155"

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: model.deviceId
                                            font.pixelSize: 12
                                            font.weight: Font.Bold
                                            color: "#F1F5F9"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: model.deviceType === 0 ? "Light" : (model.deviceType === 1 ? "Roller" : "Vacuum")
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                            color: "#34D399"
                                        }
                                    }

                                    Text {
                                        text: "Entity: " + model.topic
                                        font.pixelSize: 10
                                        color: "#94A3B8"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: "State: " + (model.isOn ? "ON" : "OFF") +
                                                  (model.deviceValue !== undefined ? " (" + Math.round(model.deviceValue * 100) + "%)" : "")
                                            font.pixelSize: 10
                                            color: "#CBD5E1"
                                        }
                                        Text {
                                            text: "Color: " + (model.deviceColor ?? "N/A")
                                            font.pixelSize: 10
                                            color: "#94A3B8"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }



                // SECTION 3: SYSTEM ACTIONS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "SYSTEM SETTINGS"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: "#F43F5E" // Rose Red
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: "Clear Credentials"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            contentItem: Text {
                                text: parent.text; font.pixelSize: 11; font.weight: Font.Bold; color: "white"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { color: parent.pressed ? "#BE123C" : "#E11D48"; radius: 6 }
                            onClicked: {
                                sensorBridge.saveHaCredentials("", "")
                                devToolsPopup.close()
                            }
                        }

                        Button {
                            text: "Reset Language"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            contentItem: Text {
                                text: parent.text; font.pixelSize: 11; font.weight: Font.Bold; color: "#0F172A"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { color: parent.pressed ? "#E2E8F0" : "#F1F5F9"; radius: 6 }
                            onClicked: {
                                sensorBridge.saveLanguage("system")
                                devToolsPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
}
