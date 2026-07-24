import QtQuick
import QtQuick.Controls

Item {
    id: root
    implicitWidth: indicatorRow.width + 24
    implicitHeight: 36

    signal clicked()

    Rectangle {
        anchors.fill: parent
        color: mouseArea.pressed ? "#1E2640" : "#151B2E"
        radius: 18
        border.color: "#232B45"
        border.width: 1

        Behavior on color { ColorAnimation { duration: 100 } }

        Row {
            id: indicatorRow
            anchors.centerIn: parent
            spacing: 8

            // Colored pulsing status dot
            Rectangle {
                id: statusDot
                width: 10
                height: 10
                radius: 5
                anchors.verticalCenter: parent.verticalCenter
                color: {
                    if (sensorBridge.haConnected) {
                        return (sensorBridge.haLatency > 250) ? "#F59E0B" : "#10B981"
                    } else if (sensorBridge.haReconnectAttempts > 0) {
                        return "#F59E0B"
                    } else {
                        return "#EF4444"
                    }
                }

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: !sensorBridge.haConnected || (sensorBridge.haConnected && sensorBridge.haLatency > 250)
                    PropertyAnimation { from: 1.0; to: 0.3; duration: 800; easing.type: Easing.InOutQuad }
                    PropertyAnimation { from: 0.3; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                }

                // If fully connected and good latency, solid opacity
                opacity: (sensorBridge.haConnected && sensorBridge.haLatency <= 250) ? 1.0 : opacity
            }

            // Latency / Status text
            Text {
                text: {
                    if (sensorBridge.haConnected) {
                        return sensorBridge.haLatency >= 0 ? sensorBridge.haLatency + " ms" : "OK"
                    } else if (sensorBridge.haReconnectAttempts > 0) {
                        return qsTr("Connecting...")
                    } else {
                        return qsTr("Offline")
                    }
                }
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: "#E2E8F0"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: root.clicked()
        }
    }
}
