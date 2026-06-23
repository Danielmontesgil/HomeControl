cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10) 

project(HomeControlCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 WebSockets)
qt_policy(SET QTP0001 NEW)
qt_standard_project_setup()

include_directories(
    include
    include/Core
    include/Devices
    include/Network
    include/UI
    include/Legacy
)

set(PROJECT_SOURCES 
    src/main.cpp 
    src/Legacy/LuminositySensor.cpp 
    src/Legacy/TemperatureSensor.cpp
    src/Legacy/AlertSystem.cpp
    src/UI/SensorBridge.cpp
    src/Network/HaWebSocketController.cpp
    src/Devices/DeviceModel.cpp
    src/Devices/DeviceFactory.cpp
    src/Devices/LightDevice.cpp
    src/Devices/RollerDevice.cpp
    include/Legacy/ISensor.h 
    include/Legacy/LuminositySensor.h 
    include/Legacy/TemperatureSensor.h 
    include/Legacy/AlertSystem.h
    include/Core/ThreadSafeValue.h
    include/Core/ConsoleLogger.h
    include/UI/SensorBridge.h
    include/Network/HaWebSocketController.h
    include/Network/IHaController.h
    include/Devices/HomeDeviceBase.h
    include/Devices/IDeviceFactory.h
    include/Devices/DeviceFactory.h
    include/Devices/DeviceModel.h
    include/Devices/IStoppable.h
    include/Devices/IValuable.h
    include/Devices/ISwitchable.h
    include/Devices/LightDevice.h
    include/Devices/RollerDevice.h
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
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::WebSockets
)

# --- Unit Testing (GoogleTest) ---
enable_testing()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(LIB_SOURCES ${PROJECT_SOURCES})
list(REMOVE_ITEM LIB_SOURCES src/main.cpp)

add_executable(HomeControlTests
    tests/ThreadSafeValueTest.cpp
    tests/RollerDeviceTest.cpp
    tests/LightDeviceTest.cpp
    tests/SensorBridgeTest.cpp
    ${LIB_SOURCES}
)

target_include_directories(HomeControlTests PRIVATE 
    include
    include/Core
    include/Devices
    include/Mqtt
    include/UI
    include/Legacy
)

target_link_libraries(HomeControlTests
    PRIVATE
    GTest::gtest_main
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::WebSockets
)

include(GoogleTest)
gtest_discover_tests(HomeControlTests)
