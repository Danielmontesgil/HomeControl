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
    property int _dishwasherCount: sensorBridge.getCountByType(3)
    property var activeVacuumDevice: null
    property var activeControlDevice: null
    property string deviceToRename: ""
    property bool showFpsOverlay: false
    property int currentFps: 60
    property bool isEditingLayout: false
    property var moduleOrder: sensorBridge.getSetting("system.module_order", "parental,lighting,covers,vacuums,dishwashers").split(",")

    property bool expandedParental: sensorBridge.internetAccess.activeCount <= 2
    property bool expandedLighting: _lightCount <= 2
    property bool expandedCovers: _rollerCount <= 2
    property bool expandedVacuums: _vacuumCount <= 2
    property bool expandedDishwashers: _dishwasherCount <= 2

    function formatDishwasherState(state, isOn, available) {
        if (!available) return qsTr("No disponible");
        if (!isOn) return qsTr("Apagado");
        if (!state) return qsTr("Listo");
        
        var s = state.toString();
        if (s.indexOf("Run") !== -1) return qsTr("Lavando");
        if (s.indexOf("Ready") !== -1) return qsTr("Listo");
        if (s.indexOf("Finished") !== -1) return qsTr("Terminado");
        if (s.indexOf("Pause") !== -1) return qsTr("Pausado");
        if (s.indexOf("Inactive") !== -1) return qsTr("Inactivo");
        if (s.indexOf("DelayedStart") !== -1) return qsTr("Inicio diferido");
        
        return s;
    }

    function formatRemainingTime(minutes) {
        if (!minutes || minutes <= 0) return "";
        var hrs = Math.floor(minutes / 60);
        var mins = minutes % 60;
        if (hrs > 0) {
            return hrs + " h " + (mins < 10 ? "0" + mins : mins) + " min";
        }
        return mins + " min";
    }

    function moveModule(name, direction) {
        var idx = moduleOrder.indexOf(name);
        if (idx === -1) return;
        var targetIdx = idx + direction;
        if (targetIdx < 0 || targetIdx >= moduleOrder.length) return;
        var arr = moduleOrder.slice();
        var temp = arr[idx];
        arr[idx] = arr[targetIdx];
        arr[targetIdx] = temp;
        moduleOrder = arr;
        sensorBridge.saveSetting("system.module_order", arr.join(","));
    }

    function getActiveLightsCount() {
        var count = 0;
        for (var i = 0; i < sensorBridge.devices.rowCount(); i++) {
            var idx = sensorBridge.devices.index(i, 0);
            var type = sensorBridge.devices.data(idx, DeviceModel.TypeRole); 
            var isOn = sensorBridge.devices.data(idx, DeviceModel.IsOnRole);
            if (type === 0 && isOn) {
                count++;
            }
        }
        return count;
    }

    function getOpenCoversCount() {
        var count = 0;
        for (var i = 0; i < sensorBridge.devices.rowCount(); i++) {
            var idx = sensorBridge.devices.index(i, 0);
            var type = sensorBridge.devices.data(idx, DeviceModel.TypeRole); 
            var val = sensorBridge.devices.data(idx, DeviceModel.ValueRole);
            if (type === 1 && val > 0.0) {
                count++;
            }
        }
        return count;
    }

    function getCleaningVacuumsCount() {
        var count = 0;
        for (var i = 0; i < sensorBridge.devices.rowCount(); i++) {
            var idx = sensorBridge.devices.index(i, 0);
            var type = sensorBridge.devices.data(idx, DeviceModel.TypeRole); 
            var state = sensorBridge.devices.data(idx, DeviceModel.VacuumStateRole);
            if (type === 2 && state === "cleaning") {
                count++;
            }
        }
        return count;
    }

    function getRunningDishwashersCount() {
        var count = 0;
        for (var i = 0; i < sensorBridge.devices.rowCount(); i++) {
            var idx = sensorBridge.devices.index(i, 0);
            var type = sensorBridge.devices.data(idx, DeviceModel.TypeRole); 
            var isOn = sensorBridge.devices.data(idx, DeviceModel.IsOnRole);
            var state = sensorBridge.devices.data(idx, DeviceModel.DishwasherStateRole);
            if (type === 3 && isOn && (state === "Run" || state === "Running")) {
                count++;
            }
        }
        return count;
    }

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
            _dishwasherCount = sensorBridge.getCountByType(3)
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

                Rectangle {
                    width: 32; height: 32; radius: 16
                    color: isEditingLayout ? "#1E3A8A" : colorCardBg
                    border.color: isEditingLayout ? colorAccent : colorBorder
                    border.width: 1
                    
                    Text {
                        anchors.centerIn: parent
                        text: isEditingLayout ? "✓" : "⚙️"
                        font.pixelSize: 14
                        color: isEditingLayout ? "white" : colorTextPrimary
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: isEditingLayout = !isEditingLayout
                    }
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

                // --- CARGADOR DINÁMICO DE MÓDULOS DEL DASHBOARD ---
                Repeater {
                    model: moduleOrder
                    delegate: Loader {
                        width: parent.width
                        height: visible ? (item ? item.implicitHeight : 0) : 0
                        visible: {
                            if (modelData === "parental") return !sensorBridge.isParentalPremium || sensorBridge.internetAccess.activeCount > 0;
                            if (modelData === "lighting") return _lightCount > 0;
                            if (modelData === "covers") return _rollerCount > 0;
                            if (modelData === "vacuums") return _vacuumCount > 0;
                            if (modelData === "dishwashers") return _dishwasherCount > 0;
                            return false;
                        }
                        sourceComponent: {
                            if (modelData === "parental") return parentalModuleComponent;
                            if (modelData === "lighting") return lightingModuleComponent;
                            if (modelData === "covers") return coversModuleComponent;
                            if (modelData === "vacuums") return vacuumsModuleComponent;
                            if (modelData === "dishwashers") return dishwashersModuleComponent;
                            return null;
                        }
                    }
                }
            }

            // --- COMPONENTES DE MÓDULOS MODULARIZADOS ---
            
            // 1. Componente: CONTROL PARENTAL
            Component {
                id: parentalModuleComponent
                Column {
                    width: parent.width
                    spacing: 12
                    visible: !sensorBridge.isParentalPremium || sensorBridge.internetAccess.activeCount > 0

                    Item {
                        width: parent.width
                        height: parentalHeaderRow.implicitHeight

                        RowLayout {
                            id: parentalHeaderRow
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: qsTr("PARENTAL CONTROL")
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                                Text {
                                    visible: !window.expandedParental && sensorBridge.isParentalPremium
                                    text: "• " + sensorBridge.internetAccess.activeCount + " online"
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                            }
                            
                            RowLayout {
                                visible: isEditingLayout
                                spacing: 4
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("parental", -1) }
                                }
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("parental", 1) }
                                }
                            }

                            Text {
                                text: window.expandedParental ? "▲" : "▼"
                                font.pixelSize: 10; color: colorAccent
                                visible: !isEditingLayout && sensorBridge.isParentalPremium
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !isEditingLayout && sensorBridge.isParentalPremium
                            onClicked: window.expandedParental = !window.expandedParental
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: window.expandedParental ? (sensorBridge.isParentalPremium ? (sensorBridge.internetAccess.activeCount * 68 - 8) : 120) : 0
                        clip: true
                        color: "transparent"
                        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

                        Item {
                            anchors.fill: parent
                            
                            Column {
                                width: parent.width
                                spacing: 8
                                visible: sensorBridge.isParentalPremium

                                Repeater {
                                    model: sensorBridge.internetAccess
                                    delegate: Rectangle {
                                        width: parent.width
                                        height: 60
                                        color: colorCardBg
                                        radius: 16
                                        border.color: colorBorder
                                        border.width: 1

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 12

                                            Rectangle {
                                                width: 36; height: 36; radius: 18; color: "#1E293D"
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: {
                                                        var t = model.type;
                                                        if (t === "smartphone") return "📱";
                                                        if (t === "television") return "📺";
                                                        if (t === "console") return "🎮";
                                                        return "🌐";
                                                    }
                                                    font.pixelSize: 16
                                                }
                                            }

                                            ColumnLayout {
                                                spacing: 1
                                                Layout.fillWidth: true
                                                Text {
                                                    text: model.name; font.pixelSize: 14; font.weight: Font.Bold; color: colorTextPrimary
                                                    elide: Text.ElideRight; Layout.fillWidth: true
                                                }
                                                Text {
                                                    text: model.isActive ? qsTr("Internet Access: Allowed") : qsTr("Internet Access: Blocked")
                                                    font.pixelSize: 11; font.weight: Font.Medium; color: model.isActive ? colorSuccess : colorDanger
                                                    Layout.fillWidth: true
                                                }
                                            }

                                            Switch {
                                                id: internetSwitch
                                                checked: model.isActive
                                                scale: 0.85
                                                indicator: Rectangle {
                                                    implicitWidth: 44; implicitHeight: 24; radius: 12
                                                    color: internetSwitch.checked ? colorSuccess : "#1F293D"
                                                    Rectangle {
                                                        x: internetSwitch.checked ? parent.width - width - 2 : 2
                                                        y: 2; width: 20; height: 20; radius: 10; color: "white"
                                                        Behavior on x { NumberAnimation { duration: 150 } }
                                                    }
                                                }
                                                onClicked: sensorBridge.toggleInternet(model.entityId, checked)
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                visible: !sensorBridge.isParentalPremium
                                color: colorCardBg
                                radius: 16; border.color: colorBorder; border.width: 1

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 8
                                    Text {
                                        text: "🔒 CONTROL PARENTAL PREMIUM"
                                        font.weight: Font.Bold; font.pixelSize: 13; color: colorTextPrimary
                                        Layout.alignment: Qt.AlignHCenter
                                    }
                                    Text {
                                        text: qsTr("Bloquea internet en consolas, TVs y móviles al instante.")
                                        font.pixelSize: 11; color: colorTextSecondary
                                        Layout.alignment: Qt.AlignHCenter
                                    }
                                    Rectangle {
                                        width: 120; height: 32; radius: 8
                                        color: "#1E2640"; border.color: colorAccent; border.width: 1
                                        Layout.alignment: Qt.AlignHCenter
                                        Text {
                                            anchors.centerIn: parent; text: qsTr("Activar Módulo")
                                            font.weight: Font.Bold; font.pixelSize: 11; color: colorAccent
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: tabBar.currentIndex = 2
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 2. Componente: ILUMINACIÓN
            Component {
                id: lightingModuleComponent
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _lightCount > 0
                    
                    Item {
                        width: parent.width
                        height: lightingHeaderRow.implicitHeight

                        RowLayout {
                            id: lightingHeaderRow
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: qsTr("LIGHTING")
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                                Text {
                                    visible: !window.expandedLighting
                                    text: "• " + getActiveLightsCount() + " active"
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                            }

                            Switch {
                                id: masterLightSwitch
                                visible: _lightCount > 1 && window.expandedLighting && !isEditingLayout
                                scale: 0.85
                                checked: sensorBridge.getCountByType(0) > 0
                                
                                indicator: Rectangle {
                                    implicitWidth: 44; implicitHeight: 24; radius: 12
                                    color: masterLightSwitch.checked ? colorSuccess : "#1F293D"
                                    Rectangle {
                                        x: masterLightSwitch.checked ? parent.width - width - 2 : 2
                                        y: 2; width: 20; height: 20; radius: 10; color: "white"
                                        Behavior on x { NumberAnimation { duration: 150 } }
                                    }
                                }
                                onClicked: sensorBridge.setAllDevicesState(0, checked ? "ON" : "OFF")
                            }

                            RowLayout {
                                visible: isEditingLayout
                                spacing: 4
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("lighting", -1) }
                                }
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("lighting", 1) }
                                }
                            }

                            Text {
                                text: window.expandedLighting ? "▲" : "▼"
                                font.pixelSize: 10; color: colorAccent
                                visible: !isEditingLayout
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !isEditingLayout
                            onClicked: window.expandedLighting = !window.expandedLighting
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: window.expandedLighting ? lightsFlow.implicitHeight : 0
                        clip: true
                        color: "transparent"
                        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

                        Flow {
                            id: lightsFlow
                            width: parent.width
                            spacing: 12
                            readonly property int columns: parent.width > 500 ? 3 : 2

                            Repeater {
                                model: sensorBridge.devices
                                delegate: Rectangle {
                                    visible: model.deviceType === 0
                                    width: visible ? (lightsFlow.width - (lightsFlow.spacing * (lightsFlow.columns - 1))) / lightsFlow.columns : 0
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

                                    Rectangle {
                                        anchors.fill: parent; anchors.margins: -4; radius: 20; color: "transparent"
                                        border.color: colorBorderActive; border.width: 1
                                        opacity: model.isOn ? 0.25 : 0.0
                                        Behavior on opacity { NumberAnimation { duration: 150 } }
                                    }

                                    Rectangle {
                                        id: refreshBtn
                                        width: 26; height: 26; radius: 13
                                        color: refreshMouseArea.containsPress ? "#3D4A6E" : (refreshMouseArea.containsMouse ? "#2C354E" : "#1A2035")
                                        border.color: "#3D4A6E"; border.width: 1
                                        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 8; z: 10
                                        visible: !model.available
                                        Text {
                                            id: refreshText; anchors.centerIn: parent; text: "🔄"; font.pixelSize: 11
                                            RotationAnimation on rotation { id: spinAnimation; from: 0; to: 360; duration: 600; running: false }
                                        }
                                        MouseArea {
                                            id: refreshMouseArea; anchors.fill: parent; hoverEnabled: true
                                            onClicked: { spinAnimation.start(); sensorBridge.forceDeviceUpdate(model.deviceId) }
                                        }
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 12; spacing: 4
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Rectangle {
                                                width: 32; height: 32; radius: 16
                                                color: model.isOn ? (model.supportsColor ? model.deviceColor : "#FFE082") : "#1F2538"
                                                Text { anchors.centerIn: parent; text: "💡"; font.pixelSize: 15; opacity: model.isOn ? 1.0 : 0.4 }
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text {
                                                text: (model.isOn && model.deviceValue !== undefined) ? Math.round(model.deviceValue * 100) + "%" : ""
                                                color: colorTextSecondary; font.pixelSize: 11; font.weight: Font.Bold
                                            }
                                        }
                                        Item { Layout.fillHeight: true }
                                        Text { text: model.deviceId; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Text {
                                            text: !model.available ? qsTr("Unavailable") : (model.isOn ? qsTr("Active") : qsTr("Off"))
                                            color: !model.available ? colorDanger : (model.isOn ? colorSuccess : colorTextSecondary); font.pixelSize: 11; Layout.fillWidth: true
                                        }
                                    }

                                    MouseArea {
                                        id: cardMouseArea; anchors.fill: parent; enabled: model.available
                                        onClicked: sensorBridge.publishCommand(model.topic, model.isOn ? "OFF" : "ON")
                                        onPressAndHold: { activeControlDevice = model; lightControlPopup.open() }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 3. Componente: PERSIANAS
            Component {
                id: coversModuleComponent
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _rollerCount > 0

                    Item {
                        width: parent.width
                        height: coversHeaderRow.implicitHeight

                        RowLayout {
                            id: coversHeaderRow
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: qsTr("BLINDS & COMFORT")
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                                Text {
                                    visible: !window.expandedCovers
                                    text: "• " + getOpenCoversCount() + " open"
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                            }

                            Slider {
                                id: masterRollerSlider
                                visible: _rollerCount > 1 && window.expandedCovers && !isEditingLayout
                                Layout.preferredWidth: 100
                                onMoved: sensorBridge.setAllDevicesState(1, Math.round(value * 100).toString())
                                handle: Rectangle {
                                    x: masterRollerSlider.leftPadding + masterRollerSlider.visualPosition * (masterRollerSlider.availableWidth - width)
                                    y: masterRollerSlider.topPadding + masterRollerSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 18; implicitHeight: 18; radius: 9
                                    color: masterRollerSlider.pressed ? colorAccent : "#FFFFFF"; border.color: colorAccent; border.width: 1
                                }
                                background: Rectangle {
                                    x: masterRollerSlider.leftPadding; y: masterRollerSlider.topPadding + masterRollerSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 100; implicitHeight: 6; width: masterRollerSlider.availableWidth; height: implicitHeight; radius: 3; color: "#1F293D"
                                    Rectangle { width: masterRollerSlider.visualPosition * parent.width; height: parent.height; color: colorAccent; radius: 3 }
                                }
                            }

                            RowLayout {
                                visible: isEditingLayout
                                spacing: 4
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("covers", -1) }
                                }
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("covers", 1) }
                                }
                            }

                            Text {
                                text: window.expandedCovers ? "▲" : "▼"
                                font.pixelSize: 10; color: colorAccent
                                visible: !isEditingLayout
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !isEditingLayout
                            onClicked: window.expandedCovers = !window.expandedCovers
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: window.expandedCovers ? rollersFlow.implicitHeight : 0
                        clip: true
                        color: "transparent"
                        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

                        Flow {
                            id: rollersFlow
                            width: parent.width
                            spacing: 12
                            readonly property int columns: parent.width > 500 ? 3 : 2

                            Repeater {
                                model: sensorBridge.devices
                                delegate: Rectangle {
                                    visible: model.deviceType === 1
                                    width: visible ? (rollersFlow.width - (rollersFlow.spacing * (rollersFlow.columns - 1))) / rollersFlow.columns : 0
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
                                        anchors.fill: parent; anchors.margins: -4; radius: 20; color: "transparent"
                                        border.color: colorWarning; border.width: 1
                                        opacity: model.isMoving ? 0.25 : 0.0
                                        Behavior on opacity { NumberAnimation { duration: 150 } }
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 12; spacing: 4
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Rectangle {
                                                width: 32; height: 32; radius: 16
                                                color: model.isMoving ? "#FFF3E0" : "#1F2538"
                                                Text { anchors.centerIn: parent; text: "🪟"; font.pixelSize: 15; opacity: model.isMoving ? 1.0 : 0.5 }
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text {
                                                text: model.available ? Math.round((model.deviceValue ?? 0) * 100) + "%" : ""
                                                color: colorTextPrimary; font.pixelSize: 12; font.weight: Font.Black
                                            }
                                        }
                                        Item { Layout.fillHeight: true }
                                        Text { text: model.deviceId; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Text {
                                            text: !model.available ? qsTr("Unavailable") : (model.isMoving ? qsTr("Moving") : qsTr("Idle"))
                                            color: !model.available ? colorDanger : (model.isMoving ? colorWarning : colorTextSecondary); font.pixelSize: 11; Layout.fillWidth: true
                                        }
                                    }

                                    MouseArea {
                                        id: rollerMouseArea; anchors.fill: parent; enabled: model.available
                                        onClicked: { activeControlDevice = model; rollerControlPopup.open() }
                                        onPressAndHold: { activeControlDevice = model; rollerControlPopup.open() }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 4. Componente: ASPIRADORES
            Component {
                id: vacuumsModuleComponent
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _vacuumCount > 0

                    Item {
                        width: parent.width
                        height: vacuumsHeaderRow.implicitHeight

                        RowLayout {
                            id: vacuumsHeaderRow
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: qsTr("VACUUMS & CLEANING")
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                                Text {
                                    visible: !window.expandedVacuums
                                    text: "• " + getCleaningVacuumsCount() + " cleaning"
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                            }

                            RowLayout {
                                visible: isEditingLayout
                                spacing: 4
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("vacuums", -1) }
                                }
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("vacuums", 1) }
                                }
                            }

                            Text {
                                text: window.expandedVacuums ? "▲" : "▼"
                                font.pixelSize: 10; color: colorAccent
                                visible: !isEditingLayout
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !isEditingLayout
                            onClicked: window.expandedVacuums = !window.expandedVacuums
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: window.expandedVacuums ? vacuumsColumn.implicitHeight : 0
                        clip: true
                        color: "transparent"
                        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

                        Column {
                            id: vacuumsColumn
                            width: parent.width
                            spacing: 8

                            Repeater {
                                model: sensorBridge.devices
                                delegate: Rectangle {
                                    visible: model.deviceType === 2
                                    width: visible ? parent.width : 0
                                    height: visible ? 130 : 0
                                    color: colorCardBg
                                    radius: 16
                                    border.color: colorBorder; border.width: 1
                                    opacity: model.available ? 1.0 : 0.4
                                    Behavior on opacity { NumberAnimation { duration: 150 } }

                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 14; spacing: 10
                                        RowLayout {
                                            Layout.fillWidth: true; spacing: 12
                                            Rectangle {
                                                width: 38; height: 38; radius: 19
                                                color: (model.vacuumState === "cleaning") ? "#E8F5E9" : "#1F2538"
                                                Text {
                                                    anchors.centerIn: parent; text: "🧹"; font.pixelSize: 18
                                                    RotationAnimation on rotation { running: model.vacuumState === "cleaning"; loops: Animation.Infinite; from: 0; to: 360; duration: 3000 }
                                                }
                                            }
                                            ColumnLayout {
                                                spacing: 2; Layout.fillWidth: true
                                                Text { text: model.deviceId ?? ""; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 15; elide: Text.ElideRight }
                                                RowLayout {
                                                    spacing: 5
                                                    Text {
                                                        text: !model.available ? qsTr("Status: Unavailable") : qsTr("Status: %1").arg(model.vacuumState ?? qsTr("unknown"))
                                                        font.pixelSize: 12; color: !model.available ? colorDanger : ((model.vacuumState === "cleaning") ? colorSuccess : colorTextSecondary); font.weight: Font.DemiBold
                                                    }
                                                    Text { text: "• " + (model.fanSpeed ?? qsTr("Standard")); font.pixelSize: 11; color: colorTextSecondary }
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
                                            Layout.fillWidth: true; spacing: 8
                                            Button {
                                                Layout.fillWidth: true; implicitHeight: 36; text: qsTr("START")
                                                enabled: model.available && model.vacuumState !== "cleaning"
                                                padding: 0
                                                background: Rectangle { color: parent.enabled ? "#1F3A2B" : "#151B2E"; border.color: parent.enabled ? colorSuccess : colorBorder; radius: 8 }
                                                contentItem: Text { text: parent.text; color: parent.enabled ? colorSuccess : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                onClicked: sensorBridge.publishCommand(model.topic, "START")
                                            }
                                            Button {
                                                Layout.fillWidth: true; implicitHeight: 36; text: qsTr("PAUSE")
                                                enabled: model.available && model.vacuumState === "cleaning"
                                                padding: 0
                                                background: Rectangle { color: parent.enabled ? "#3D2B1F" : "#151B2E"; border.color: parent.enabled ? colorWarning : colorBorder; radius: 8 }
                                                contentItem: Text { text: parent.text; color: parent.enabled ? colorWarning : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                onClicked: sensorBridge.publishCommand(model.topic, "PAUSE")
                                            }
                                            Button {
                                                Layout.fillWidth: true; implicitHeight: 36; text: qsTr("DOCK")
                                                enabled: model.available && model.vacuumState !== "docked" && model.vacuumState !== "returning"
                                                padding: 0
                                                background: Rectangle { color: parent.enabled ? "#1F2E3D" : "#151B2E"; border.color: parent.enabled ? colorAccent : colorBorder; radius: 8 }
                                                contentItem: Text { text: parent.text; color: parent.enabled ? colorAccent : "#475569"; font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                onClicked: sensorBridge.publishCommand(model.topic, "RETURN")
                                            }
                                            Button {
                                                implicitWidth: 36; implicitHeight: 36; text: "🗺️"; padding: 0
                                                background: Rectangle { color: "#1F293D"; border.color: colorBorder; radius: 8 }
                                                contentItem: Text { text: parent.text; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                                onClicked: { activeVacuumDevice = model; vacuumDetailsPopup.open() }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 5. Componente: LAVAVAJILLAS
            Component {
                id: dishwashersModuleComponent
                Column {
                    width: parent.width
                    spacing: 12
                    visible: _dishwasherCount > 0

                    Item {
                        width: parent.width
                        height: dishwashersHeaderRow.implicitHeight

                        RowLayout {
                            id: dishwashersHeaderRow
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: qsTr("DISHWASHERS")
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                                Text {
                                    visible: !window.expandedDishwashers
                                    text: "• " + getRunningDishwashersCount() + " active"
                                    font.pixelSize: 11; font.weight: Font.Bold; color: colorTextSecondary
                                }
                            }

                            RowLayout {
                                visible: isEditingLayout
                                spacing: 4
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("dishwashers", -1) }
                                }
                                Rectangle {
                                    width: 24; height: 24; radius: 4
                                    color: "#1E293D"; border.color: colorBorder
                                    Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 10; color: colorTextPrimary }
                                    MouseArea { anchors.fill: parent; onClicked: moveModule("dishwashers", 1) }
                                }
                            }

                            Text {
                                text: window.expandedDishwashers ? "▲" : "▼"
                                font.pixelSize: 10; color: colorAccent
                                visible: !isEditingLayout
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !isEditingLayout
                            onClicked: window.expandedDishwashers = !window.expandedDishwashers
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: window.expandedDishwashers ? dishwashersColumn.implicitHeight : 0
                        clip: true
                        color: "transparent"
                        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

                        Column {
                            id: dishwashersColumn
                            width: parent.width
                            spacing: 8

                            Repeater {
                                model: sensorBridge.devices
                                delegate: Rectangle {
                                     id: dishwasherCard
                                     visible: model.deviceType === 3
                                     width: visible ? parent.width : 0
                                     height: visible ? 130 : 0
                                     color: model.isOn ? colorCardBgActive : colorCardBg
                                     radius: 16
                                     border.color: model.isOn ? colorBorderActive : colorBorder
                                     border.width: 1
                                     opacity: model.available ? 1.0 : 0.4
                                     Behavior on opacity { NumberAnimation { duration: 150 } }
                                     Behavior on color { ColorAnimation { duration: 150 } }

                                     scale: cardMouseArea.pressed ? 0.98 : 1.0
                                     Behavior on scale { NumberAnimation { duration: 80 } }

                                     ColumnLayout {
                                         anchors.fill: parent; anchors.margins: 14; spacing: 10

                                         // Fila Superior: Info General
                                         RowLayout {
                                             Layout.fillWidth: true
                                             spacing: 12
                                             
                                             Rectangle {
                                                 width: 36; height: 36; radius: 18
                                                 color: model.dishwasherDoorOpen ? "#3D1F1F" : ((model.dishwasherState !== undefined && model.dishwasherState !== null && model.dishwasherState.toString().indexOf("Run") !== -1) ? "#1F3A2B" : "#1F2538")
                                                 Text { anchors.centerIn: parent; text: model.dishwasherDoorOpen ? "🚪" : "🍽️"; font.pixelSize: 16 }
                                             }

                                             ColumnLayout {
                                                 spacing: 1; Layout.fillWidth: true
                                                 Text { text: model.deviceId ?? ""; color: colorTextPrimary; font.weight: Font.Bold; font.pixelSize: 14; elide: Text.ElideRight }
                                                 Text {
                                                     text: formatDishwasherState(model.dishwasherState, model.isOn, model.available) + (model.dishwasherDoorOpen ? qsTr(" (Puerta abierta)") : "")
                                                     font.pixelSize: 11
                                                     color: !model.available || model.dishwasherDoorOpen ? colorDanger : 
                                                            ((model.dishwasherState !== undefined && model.dishwasherState !== null && model.dishwasherState.toString().indexOf("Run") !== -1) ? colorSuccess : colorTextSecondary)
                                                     font.weight: Font.DemiBold
                                                 }
                                             }

                                             ColumnLayout {
                                                 spacing: 1; Layout.alignment: Qt.AlignRight
                                                 Text {
                                                     text: (model.available && model.isOn && model.dishwasherRemainingTime > 0) ? formatRemainingTime(model.dishwasherRemainingTime) : ""
                                                     font.pixelSize: 13; font.weight: Font.Black; color: colorSuccess; Layout.alignment: Qt.AlignRight
                                                 }
                                                 Text {
                                                     text: (model.available && model.isOn && model.dishwasherProgram) ? (model.dishwasherProgram.toString().indexOf("Eco50") !== -1 ? "Eco 50°C" : "Programa") : ""
                                                     font.pixelSize: 10; color: colorTextSecondary; Layout.alignment: Qt.AlignRight
                                                 }
                                             }
                                         }

                                         // Fila Inferior: Botones de Acción Rápida
                                         RowLayout {
                                             Layout.fillWidth: true
                                             spacing: 8
                                             visible: model.available

                                             // Botón rápido de Encendido
                                             Button {
                                                 implicitHeight: 32
                                                 Layout.preferredWidth: 100
                                                 text: model.isOn ? qsTr("APAGAR") : qsTr("ENCENDER")
                                                 padding: 0
                                                 background: Rectangle {
                                                     color: model.isOn ? "#3D2B1F" : "#1F2E3D"
                                                     border.color: model.isOn ? colorWarning : colorAccent
                                                     border.width: 1
                                                     radius: 8
                                                 }
                                                 contentItem: Text {
                                                     text: parent.text
                                                     color: model.isOn ? colorWarning : colorAccent
                                                     font.weight: Font.Bold; font.pixelSize: 10
                                                     horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                                 }
                                                 onClicked: {
                                                     sensorBridge.publishCommand(model.topic, model.isOn ? "OFF" : "ON")
                                                 }
                                             }

                                             // Botón rápido de Ciclo
                                             Button {
                                                 implicitHeight: 32
                                                 Layout.preferredWidth: 100
                                                 readonly property bool isRunning: (model.dishwasherState !== undefined && model.dishwasherState !== null) ? model.dishwasherState.toString().indexOf("Run") !== -1 : false
                                                 text: isRunning ? qsTr("DETENER") : qsTr("INICIAR")
                                                 enabled: model.isOn && (!isRunning ? !model.dishwasherDoorOpen : true)
                                                 padding: 0
                                                 background: Rectangle {
                                                     color: parent.enabled ? (parent.isRunning ? "#3D1F1F" : "#1F3A2B") : "#151B2E"
                                                     border.color: parent.enabled ? (parent.isRunning ? colorDanger : colorSuccess) : colorBorder
                                                     border.width: 1
                                                     radius: 8
                                                 }
                                                 contentItem: Text {
                                                     text: parent.text
                                                     color: parent.enabled ? (parent.isRunning ? colorDanger : colorSuccess) : "#475569"
                                                     font.weight: Font.Bold; font.pixelSize: 10
                                                     horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                                 }
                                                 onClicked: {
                                                     sensorBridge.publishCommand(model.topic, isRunning ? "STOP" : "START")
                                                 }
                                             }

                                             Item { Layout.fillWidth: true }

                                             // Programa Seleccionado a la derecha
                                             Text {
                                                 visible: model.isOn && model.dishwasherProgram
                                                 text: model.dishwasherProgram ? (model.dishwasherProgram.toString().indexOf("Eco50") !== -1 ? "Eco 50°C" : "Personalizado") : ""
                                                 font.pixelSize: 11
                                                 font.weight: Font.DemiBold
                                                 color: colorTextSecondary
                                                 Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                             }
                                         }
                                     }

                                     MouseArea {
                                         id: cardMouseArea
                                         width: parent.width
                                         height: 75
                                         anchors.top: parent.top
                                         enabled: model.available
                                         onPressAndHold: { activeControlDevice = model; dishwasherControlPopup.open() }
                                     }
                                 }
                            }
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
        height: 360
        y: parent.height - height
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

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
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            anchors.bottomMargin: 32
            spacing: 14

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

            // 1. Slider de Brillo iOS-Style
            ColumnLayout {
                Layout.fillWidth: true
                visible: activeControlDevice && activeControlDevice.deviceValue !== undefined
                spacing: 4
                Label { text: qsTr("BRIGHTNESS"); font.pixelSize: 10; color: colorTextSecondary; font.weight: Font.Bold }
                Slider {
                    id: brightnessSlider
                    Layout.fillWidth: true
                    value: (activeControlDevice && activeControlDevice.deviceValue !== undefined) ? activeControlDevice.deviceValue : 0.0
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

            // 2. Slider de Temperatura Kelvin (Blanco Regulable)
            ColumnLayout {
                Layout.fillWidth: true
                visible: activeControlDevice && activeControlDevice.supportsColorTemp
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("COLOR TEMPERATURE"); font.pixelSize: 10; color: colorTextSecondary; font.weight: Font.Bold }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: (activeControlDevice && activeControlDevice.colorTemp !== undefined ? activeControlDevice.colorTemp : 4000) + " K"
                        font.pixelSize: 11; font.weight: Font.Bold; color: colorTextPrimary
                    }
                }
                Slider {
                    id: kelvinSlider
                    Layout.fillWidth: true
                    from: (activeControlDevice && activeControlDevice.minColorTemp !== undefined) ? activeControlDevice.minColorTemp : 2000
                    to: (activeControlDevice && activeControlDevice.maxColorTemp !== undefined) ? activeControlDevice.maxColorTemp : 6500
                    value: (activeControlDevice && activeControlDevice.colorTemp !== undefined) ? activeControlDevice.colorTemp : 4000
                    stepSize: 50
                    onPressedChanged: {
                        if (!pressed && activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, "KELVIN:" + Math.round(value))
                        }
                    }
                    background: Rectangle {
                        implicitHeight: 24; radius: 12
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#FF9E2A" } // Cálido
                            GradientStop { position: 0.5; color: "#FFFFEF" } // Neutro
                            GradientStop { position: 1.0; color: "#A8CEFF" } // Frío
                        }
                    }
                    handle: Rectangle {
                        x: kelvinSlider.leftPadding + kelvinSlider.visualPosition * (kelvinSlider.availableWidth - width)
                        y: kelvinSlider.topPadding + kelvinSlider.availableHeight / 2 - height / 2
                        implicitWidth: 32; implicitHeight: 32; radius: 16; color: "#FFFFFF"; border.color: "#E2E8F0"; border.width: 2
                    }
                }
            }

            // 3. Presets de Color
            ColumnLayout {
                Layout.fillWidth: true
                visible: activeControlDevice && activeControlDevice.supportsColor
                spacing: 4
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
                    value: (activeControlDevice && activeControlDevice.deviceValue !== undefined) ? activeControlDevice.deviceValue : 0.0
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

    // 4. Popup Detalles del Lavavajillas (Opciones + Programas + Controles)
    Popup {
        id: dishwasherControlPopup
        width: parent.width
        height: 400
        y: parent.height - height
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        enter: Transition {
            NumberAnimation { property: "y"; from: window.height; to: window.height - 400; duration: 250; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "y"; from: window.height - 400; to: window.height; duration: 200; easing.type: Easing.InCubic }
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
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            anchors.bottomMargin: 32
            spacing: 14

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
                    onClicked: dishwasherControlPopup.close()
                }
            }

            // 1. Fila de Estado: Operación + Puerta
            RowLayout {
                Layout.fillWidth: true; spacing: 16
                Rectangle {
                    color: "#1A2035"; radius: 8
                    Layout.fillWidth: true; implicitHeight: 45
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10; spacing: 8
                        Text { text: "🚪"; font.pixelSize: 16 }
                        Text {
                            text: (activeControlDevice && activeControlDevice.dishwasherDoorOpen) ? qsTr("Puerta abierta") : qsTr("Puerta cerrada")
                            color: (activeControlDevice && activeControlDevice.dishwasherDoorOpen) ? colorDanger : colorTextSecondary
                            font.pixelSize: 12; font.weight: Font.Bold
                        }
                    }
                }
                Rectangle {
                    color: "#1A2035"; radius: 8
                    Layout.fillWidth: true; implicitHeight: 45
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10; spacing: 8
                        Text { text: "⚙️"; font.pixelSize: 16 }
                        Text {
                            text: activeControlDevice ? formatDishwasherState(activeControlDevice.dishwasherState, activeControlDevice.isOn, activeControlDevice.available) : ""
                            color: (activeControlDevice && (activeControlDevice.dishwasherState === "Run" || activeControlDevice.dishwasherState === "Running")) ? colorSuccess : colorTextPrimary
                            font.pixelSize: 12; font.weight: Font.Bold
                        }
                    }
                }
            }

            // 2. Información del programa y tiempo restante
            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                visible: activeControlDevice && activeControlDevice.isOn
                
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: qsTr("PROGRAMA SELECCIONADO"); font.pixelSize: 10; font.weight: Font.Bold; color: colorTextSecondary }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: (activeControlDevice && activeControlDevice.dishwasherRemainingTime > 0) ? qsTr("%1 restante").arg(formatRemainingTime(activeControlDevice.dishwasherRemainingTime)) : ""
                        font.pixelSize: 11; font.weight: Font.Bold; color: colorSuccess
                    }
                }

                ComboBox {
                    id: programComboBox
                    Layout.fillWidth: true
                    implicitHeight: 45
                    model: [
                        { text: "Eco 50°C", value: "Dishcare.Dishwasher.Program.Eco50" },
                        { text: "Auto 45-65°C", value: "Dishcare.Dishwasher.Program.Auto2" },
                        { text: "Intensive 70°C", value: "Dishcare.Dishwasher.Program.Intensiv70" },
                        { text: "Quick wash 65°C", value: "Dishcare.Dishwasher.Program.Quick65" },
                        { text: "Night Wash", value: "Dishcare.Dishwasher.Program.NightWash" },
                        { text: "Machine Care", value: "Dishcare.Dishwasher.Program.MachineCare" },
                        { text: "Pre-rinse", value: "Dishcare.Dishwasher.Program.PreRinse" }
                    ]
                    textRole: "text"
                    valueRole: "value"

                    property string currentDbProgram: (activeControlDevice && activeControlDevice.dishwasherProgram) ? activeControlDevice.dishwasherProgram.toString() : ""

                    onCurrentDbProgramChanged: {
                        if (!popup.visible) {
                            updateIndexFromNetwork();
                        }
                    }

                    function updateIndexFromNetwork() {
                        if (!activeControlDevice || !activeControlDevice.dishwasherProgram) {
                            currentIndex = -1;
                            return;
                        }
                        var curProg = activeControlDevice.dishwasherProgram.toString();
                        for (var i = 0; i < model.length; i++) {
                            if (model[i].value === curProg) {
                                currentIndex = i;
                                return;
                            }
                        }
                        currentIndex = -1;
                    }

                    Connections {
                        target: dishwasherControlPopup
                        function onOpened() {
                            programComboBox.updateIndexFromNetwork();
                        }
                    }

                    onActivated: {
                        if (activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, "SELECT_PROGRAM:" + model[index].value);
                        }
                    }

                    contentItem: RowLayout {
                        spacing: 8
                        Text { text: "🍽️"; font.pixelSize: 18; Layout.leftMargin: 12 }
                        Text {
                            text: programComboBox.displayText
                            color: colorTextPrimary
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    background: Rectangle {
                        color: "#1A2035"
                        border.color: colorBorder
                        border.width: 1
                        radius: 10
                    }

                    popup: Popup {
                        y: programComboBox.height + 4
                        width: programComboBox.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: programComboBox.popup.visible ? programComboBox.delegateModel : null
                            currentIndex: programComboBox.highlightedIndex

                            ScrollIndicator.vertical: ScrollIndicator { }
                        }

                        background: Rectangle {
                            color: "#111827"
                            border.color: colorBorder
                            border.width: 1
                            radius: 12
                        }
                    }

                    delegate: ItemDelegate {
                        width: programComboBox.width
                        height: 40
                        highlighted: programComboBox.highlightedIndex === index
                        
                        contentItem: Text {
                            text: modelData.text
                            color: highlighted ? colorAccent : colorTextPrimary
                            font.weight: highlighted ? Font.Bold : Font.Normal
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 16
                        }

                        background: Rectangle {
                            color: highlighted ? "#1E293B" : "transparent"
                            radius: 8
                        }
                    }
                }
            }

            // 3. Fila de Opciones Adicionales
            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                visible: activeControlDevice && activeControlDevice.isOn
                Text { text: qsTr("OPCIONES ADICIONALES"); font.pixelSize: 10; font.weight: Font.Bold; color: colorTextSecondary }
                
                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    
                    // ExtraDry Toggle
                    Rectangle {
                        color: "#1A2035"; radius: 10; border.color: (activeControlDevice && activeControlDevice.dishwasherExtraDry) ? colorAccent : colorBorder; border.width: 1
                        Layout.fillWidth: true; implicitHeight: 48
                        MouseArea {
                            anchors.fill: parent
                            onClicked: activeControlDevice && sensorBridge.publishCommand(activeControlDevice.topic, "TOGGLE_EXTRADRY")
                        }
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10
                            ColumnLayout {
                                spacing: 1
                                Text { text: qsTr("ExtraDry"); color: colorTextPrimary; font.pixelSize: 11; font.weight: Font.Bold }
                                Text { text: qsTr("Secado Extra"); color: colorTextSecondary; font.pixelSize: 9 }
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 14; height: 14; radius: 7
                                color: (activeControlDevice && activeControlDevice.dishwasherExtraDry) ? colorAccent : "transparent"
                                border.color: (activeControlDevice && activeControlDevice.dishwasherExtraDry) ? colorAccent : colorBorder; border.width: 2
                            }
                        }
                    }

                    // Half Load Toggle
                    Rectangle {
                        color: "#1A2035"; radius: 10; border.color: (activeControlDevice && activeControlDevice.dishwasherHalfLoad) ? colorAccent : colorBorder; border.width: 1
                        Layout.fillWidth: true; implicitHeight: 48
                        MouseArea {
                            anchors.fill: parent
                            onClicked: activeControlDevice && sensorBridge.publishCommand(activeControlDevice.topic, "TOGGLE_HALFLOAD")
                        }
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10
                            ColumnLayout {
                                spacing: 1
                                Text { text: qsTr("Half Load"); color: colorTextPrimary; font.pixelSize: 11; font.weight: Font.Bold }
                                Text { text: qsTr("Media Carga"); color: colorTextSecondary; font.pixelSize: 9 }
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 14; height: 14; radius: 7
                                color: (activeControlDevice && activeControlDevice.dishwasherHalfLoad) ? colorAccent : "transparent"
                                border.color: (activeControlDevice && activeControlDevice.dishwasherHalfLoad) ? colorAccent : colorBorder; border.width: 2
                            }
                        }
                    }

                    // SpeedPerfect+ Toggle
                    Rectangle {
                        color: "#1A2035"; radius: 10; border.color: (activeControlDevice && activeControlDevice.dishwasherSpeedPerfect) ? colorAccent : colorBorder; border.width: 1
                        Layout.fillWidth: true; implicitHeight: 48
                        MouseArea {
                            anchors.fill: parent
                            onClicked: activeControlDevice && sensorBridge.publishCommand(activeControlDevice.topic, "TOGGLE_SPEEDPERFECT")
                        }
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10
                            ColumnLayout {
                                spacing: 1
                                Text { text: qsTr("SpeedPerfect+"); color: colorTextPrimary; font.pixelSize: 11; font.weight: Font.Bold }
                                Text { text: qsTr("Ciclo Rápido"); color: colorTextSecondary; font.pixelSize: 9 }
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 14; height: 14; radius: 7
                                color: (activeControlDevice && activeControlDevice.dishwasherSpeedPerfect) ? colorAccent : "transparent"
                                border.color: (activeControlDevice && activeControlDevice.dishwasherSpeedPerfect) ? colorAccent : colorBorder; border.width: 2
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // 4. Botones Principales de Ciclo e Inicio
            RowLayout {
                Layout.fillWidth: true; spacing: 10
                
                Button {
                    Layout.fillWidth: true; implicitHeight: 42; text: (activeControlDevice && activeControlDevice.isOn) ? qsTr("APAGAR MÁQUINA") : qsTr("ENCENDER MÁQUINA")
                    enabled: activeControlDevice ? activeControlDevice.available : false
                    padding: 0
                    background: Rectangle {
                        color: parent.enabled ? ((activeControlDevice && activeControlDevice.isOn) ? "#3D2B1F" : "#1F2E3D") : "#151B2E"
                        border.color: parent.enabled ? ((activeControlDevice && activeControlDevice.isOn) ? colorWarning : colorAccent) : colorBorder
                        radius: 10
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? ((activeControlDevice && activeControlDevice.isOn) ? colorWarning : colorAccent) : "#475569"
                        font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, activeControlDevice.isOn ? "OFF" : "ON")
                        }
                    }
                }

                // Iniciar / Detener
                Button {
                    Layout.fillWidth: true; implicitHeight: 42
                    property bool isRunning: activeControlDevice && activeControlDevice.dishwasherState && activeControlDevice.dishwasherState.toString().indexOf("Run") !== -1
                    text: isRunning ? qsTr("DETENER CICLO") : qsTr("INICIAR PROGRAMA")
                    enabled: activeControlDevice && activeControlDevice.available && activeControlDevice.isOn && (!isRunning ? !activeControlDevice.dishwasherDoorOpen : true)
                    padding: 0
                    background: Rectangle {
                        color: parent.enabled ? (parent.isRunning ? "#3D1F1F" : "#1F3A2B") : "#151B2E"
                        border.color: parent.enabled ? (parent.isRunning ? colorDanger : colorSuccess) : colorBorder
                        radius: 10
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? (parent.isRunning ? colorDanger : colorSuccess) : "#475569"
                        font.weight: Font.Bold; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (activeControlDevice) {
                            sensorBridge.publishCommand(activeControlDevice.topic, isRunning ? "STOP" : "START")
                        }
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
        // Sección de Licencia Premium
        ColumnLayout {
            Layout.fillWidth: true; spacing: 8
            Label { text: qsTr("Feature Activation:"); font.weight: Font.Bold; color: colorTextPrimary }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                
                TextField {
                    id: licenseKeyInput
                    Layout.fillWidth: true
                    placeholderText: sensorBridge.isParentalPremium ? qsTr("Module activated") : qsTr("Enter license key...")
                    placeholderTextColor: colorTextSecondary
                    text: sensorBridge.isParentalPremium ? "DEMO-PARENTAL-KEY-2026" : ""
                    enabled: !sensorBridge.isParentalPremium
                    color: colorTextPrimary
                    
                    background: Rectangle {
                        implicitHeight: 40
                        color: colorCardBg
                        border.color: colorBorder
                        radius: 6
                    }
                }
                
                Button {
                    text: sensorBridge.isParentalPremium ? qsTr("Activated") : qsTr("Activate")
                    enabled: !sensorBridge.isParentalPremium && licenseKeyInput.text.trim() !== ""
                    implicitHeight: 40
                    Layout.preferredWidth: 100
                    
                    contentItem: Text {
                        text: parent.text
                        font.weight: Font.Bold
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.pressed ? "#1D4ED8" : colorAccent) : "#1E293B"
                        radius: 6
                    }
                    
                    onClicked: {
                        var success = sensorBridge.activateParentalControl(licenseKeyInput.text.trim())
                        if (success) {
                            licenseStatusText.text = qsTr("Parental Control Premium successfully activated!")
                            licenseStatusText.color = colorSuccess
                        } else {
                            licenseStatusText.text = qsTr("Invalid license key.")
                            licenseStatusText.color = colorDanger
                        }
                    }
                }
            }
            Text {
                id: licenseStatusText
                text: sensorBridge.isParentalPremium ? qsTr("Parental Control Premium is active.") : ""
                font.pixelSize: 11
                color: sensorBridge.isParentalPremium ? colorSuccess : colorDanger
                Layout.alignment: Qt.AlignLeft
            }
        }

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
