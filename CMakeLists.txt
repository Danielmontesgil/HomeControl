cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10) 

project(HomeControlCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
qt_policy(SET QTP0001 NEW)
qt_standard_project_setup()

# --- Integracion Nativa de MQTT (Arquitectura de Subproyectos) ---
include(FetchContent)

FetchContent_Declare(
        paho_mqtt_cpp
        GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.cpp
        GIT_TAG        v1.6.0
)

set(PAHO_WITH_MQTT_C ON CACHE INTERNAL "")
set(PAHO_BUILD_STATIC ON CACHE INTERNAL "")
set(PAHO_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(PAHO_WITH_SSL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(paho_mqtt_cpp)
include_directories(
    include
    include/Core
    include/Devices
    include/Mqtt
    include/UI
    include/Legacy
)

set(PROJECT_SOURCES 
        src/main.cpp 
        include/Legacy/ISensor.h 
        include/Legacy/LuminositySensor.h 
        src/Legacy/LuminositySensor.cpp 
        include/Legacy/TemperatureSensor.h 
        src/Legacy/TemperatureSensor.cpp
        include/Legacy/AlertSystem.h
        src/Legacy/AlertSystem.cpp
        include/Core/ThreadSafeValue.h
        include/Core/ConsoleLogger.h
        include/UI/SensorBridge.h
        src/UI/SensorBridge.cpp
        include/Mqtt/IMqttController.h
        include/Mqtt/MqttController.h
        src/Mqtt/MqttController.cpp
        include/Mqtt/IMqttListener.h
        include/Devices/HomeDeviceBase.h
        include/Devices/IDeviceFactory.h
        include/Devices/DeviceFactory.h
        src/Devices/DeviceFactory.cpp
        include/Devices/LightDevice.h
        src/Devices/LightDevice.cpp
        include/Devices/RollerDevice.h
        src/Devices/RollerDevice.cpp
        include/Devices/DeviceModel.h
        src/Devices/DeviceModel.cpp
)

add_executable(HomeControl ${PROJECT_SOURCES})

qt_add_qml_module(HomeControl
        URI SensorsApp
        VERSION 1.0
        RESOURCE_PREFIX "/"
        QML_FILES main.qml
)

target_link_libraries(HomeControl 
    PRIVATE 
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick
    paho-mqttpp3-static
)

# --- Unit Testing (GoogleTest) ---
enable_testing()

# Descargamos GoogleTest (GTest) desde su repositorio oficial
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# Forzar a GTest a usar la misma configuración de MSVC que tu proyecto
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Preparamos los archivos fuente para los tests (excluimos main.cpp)
set(LIB_SOURCES ${PROJECT_SOURCES})
list(REMOVE_ITEM LIB_SOURCES src/main.cpp)

# Creamos el ejecutable de los tests
add_executable(HomeControlTests
    tests/ThreadSafeValueTest.cpp
    tests/RollerDeviceTest.cpp
    tests/LightDeviceTest.cpp
    tests/MqttControllerTest.cpp
    tests/SensorBridgeTest.cpp
    ${LIB_SOURCES}
)

# Le decimos a los tests dónde encontrar tus clases de C++
target_include_directories(HomeControlTests PRIVATE 
    include
    include/Core
    include/Devices
    include/Mqtt
    include/UI
    include/Legacy
)

# Enlazamos la librería de GTest, Qt y Paho
target_link_libraries(HomeControlTests
    PRIVATE
    GTest::gtest_main
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick
    paho-mqttpp3-static
)

# Activamos el descubrimiento automático de tests para CTest/Rider
include(GoogleTest)
gtest_discover_tests(HomeControlTests)
