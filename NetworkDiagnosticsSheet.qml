import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: popup
    parent: Overlay.overlay
    width: Math.min(window.width - 40, 380)
    height: contentColumn.implicitHeight + 40
    modal: true
    focus: true
    anchors.centerIn: parent

    property int remainingMs: 0
    signal openDevTools()

    Timer {
        interval: 100
        running: popup.visible && !sensorBridge.haConnected
        repeat: true
        onTriggered: popup.remainingMs = sensorBridge.haNextReconnectDelay
    }

    padding: 0

    background: Rectangle {
        color: "white"
        radius: 16
        border.color: "#E2E8F0"
        border.width: 1
    }

    contentItem: Rectangle {
        color: "white"
        radius: 16
        border.color: "#E2E8F0"
        border.width: 1

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

        // Title Row
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Network Health")
                font.pixelSize: 20
                font.weight: Font.Bold
                color: "#1A237E"
                Layout.fillWidth: true
            }
            Button {
                text: "✕"
                flat: true
                implicitWidth: 32; implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "#7F8C8D"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: popup.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#E2E8F0"
        }

        // Connection State Status
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Rectangle {
                width: 40; height: 40; radius: 20
                color: sensorBridge.haConnected ? "#E8F8F5" : "#FDEDEC"
                Text {
                    anchors.centerIn: parent
                    text: sensorBridge.haConnected ? "⚡" : "❌"
                    font.pixelSize: 18
                }
            }
            ColumnLayout {
                Text {
                    text: sensorBridge.haConnected ? qsTr("Connected") : qsTr("Disconnected")
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    color: sensorBridge.haConnected ? "#27AE60" : "#C0392B"
                }
                Text {
                    text: sensorBridge.haConnected ? qsTr("Home Assistant Link active") : qsTr("Searching for server...")
                    font.pixelSize: 12
                    color: "#7F8C8D"
                }
            }
        }

        // Metrics Table
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            // Latency (RTT)
            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("Network Latency:"); font.pixelSize: 13; color: "#4A5568"; Layout.fillWidth: true }
                Text {
                    text: sensorBridge.haConnected ? (sensorBridge.haLatency >= 0 ? sensorBridge.haLatency + " ms" : qsTr("Measuring...")) : "--"
                    font.pixelSize: 13; font.weight: Font.Bold; color: "#2D3748"
                }
            }

            // Reconnection Attempts
            RowLayout {
                Layout.fillWidth: true
                visible: !sensorBridge.haConnected
                Text { text: qsTr("Reconnect Attempts:"); font.pixelSize: 13; color: "#4A5568"; Layout.fillWidth: true }
                Text {
                    text: sensorBridge.haReconnectAttempts
                    font.pixelSize: 13; font.weight: Font.Bold; color: "#2D3748"
                }
            }

            // Reconnection Timer countdown
            ColumnLayout {
                Layout.fillWidth: true
                visible: !sensorBridge.haConnected && popup.remainingMs > 0
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: qsTr("Next Reconnect in:"); font.pixelSize: 13; color: "#4A5568"; Layout.fillWidth: true }
                    Text {
                        text: (popup.remainingMs / 1000.0).toFixed(1) + " s"
                        font.pixelSize: 13; font.weight: Font.Bold; color: "#1A237E"
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    value: popup.remainingMs
                    to: sensorBridge.haNextReconnectDelay > 0 ? sensorBridge.haNextReconnectDelay : 30000
                    background: Rectangle { implicitHeight: 6; color: "#E2E8F0"; radius: 3 }
                    contentItem: Item {
                        Rectangle {
                            width: parent.width
                            height: parent.height
                            color: "#1A237E"
                            radius: 3
                        }
                    }
                }
            }

            // Last Disconnect Reason
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text { text: qsTr("Last Disconnect Reason:"); font.pixelSize: 12; color: "#718096" }
                Text {
                    text: sensorBridge.haLastDisconnectReason
                    font.pixelSize: 12; font.weight: Font.Medium; color: "#E53E3E"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // Action Buttons
        Button {
            text: qsTr("Reconnect Now")
            Layout.fillWidth: true
            implicitHeight: 44
            visible: !sensorBridge.haConnected
            contentItem: Text {
                text: parent.text
                font.weight: Font.Bold
                font.pixelSize: 14
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.pressed ? "#0D47A1" : "#1A237E"
                radius: 8
            }
            onClicked: {
                sensorBridge.reconnect()
            }
        }

        Button {
            text: qsTr("🛠️ Developer Tools")
            Layout.fillWidth: true
            implicitHeight: 44
            visible: sensorBridge.isDebugBuild
            contentItem: Text {
                text: parent.text
                font.weight: Font.Bold
                font.pixelSize: 14
                color: "#1A237E"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.pressed ? "#E2E8F0" : "#F7FAFC"
                border.color: "#1A237E"
                border.width: 1
                radius: 8
            }
            onClicked: {
                popup.close()
                popup.openDevTools()
            }
        }
    }
}
}
