import QtQuick

Window {
    visible: true
    width: 400
    height: 600
    title: "Eltex Sensor Monitor"
    color: "#2c3e50"

    Column {
        anchors.centerIn: parent
        spacing: 30
        width: parent.width

        Text {
            text: "Panel de Control Eltex"
            color: "#ecf0f1"
            font.pixelSize: 28
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Bloque Estado de Luz
        Column {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter
            Text { text: "Estado de Luz:"; color: "#95a5a6"; font.pixelSize: 16; anchors.horizontalCenter: parent.horizontalCenter }
            Text {
                // Comprobacion de seguridad y conversion a texto legible
                text: sensorBridge ? (sensorBridge.lightStatus > 0.5 ? "ENCENDIDA" : "APAGADA") : "---"
                color: sensorBridge && sensorBridge.lightStatus > 0.5 ? "#f1c40f" : "#7f8c8d"
                font.pixelSize: 32
                font.family: "Monospace"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // Bloque Posicion Persiana
        Column {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter
            Text { text: "Apertura Persiana:"; color: "#95a5a6"; font.pixelSize: 16; anchors.horizontalCenter: parent.horizontalCenter }
            Text {
                // Mostramos el porcentaje (0.5 -> 50%)
                text: sensorBridge ? (sensorBridge.rollerPosition * 100).toFixed(0) + "%" : "0%"
                color: "#3498db"
                font.pixelSize: 32
                font.family: "Monospace"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // --- BOTÓN DE CONTROL ---
        Rectangle {
            id: lightButton
            width: 200; height: 60
            color: sensorBridge && sensorBridge.lightStatus > 0.5 ? "#f1c40f" : "#34495e"
            radius: 10
            anchors.horizontalCenter: parent.horizontalCenter
            border.color: "white"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: sensorBridge && sensorBridge.lightStatus > 0.5 ? "APAGAR" : "ENCENDER"
                color: sensorBridge && sensorBridge.lightStatus > 0.5 ? "black" : "white"
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (sensorBridge) {
                        // Invertimos el estado actual
                        sensorBridge.toggleLight(sensorBridge.lightStatus < 0.5);
                    }
                }
            }
        }
    }
}
