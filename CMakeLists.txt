cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10) 

project(HomeControlCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 WebSockets Network LinguistTools)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_standard_project_setup()

include_directories(
    include
    include/Core
    include/Devices
    include/Network
    include/UI
    include/Commands
    include/Bootstrap
)

set(PROJECT_SOURCES 
    src/main.cpp 
    src/UI/SensorBridge.cpp
    src/Network/HaWebSocketController.cpp
    src/Devices/DeviceModel.cpp
    src/Devices/DeviceFactory.cpp
    src/Commands/GenericHaCommand.cpp
    src/Bootstrap/AppSetup.cpp
    include/Commands/ICommand.h
    include/Commands/GenericHaCommand.h
    include/Bootstrap/AppSetup.h
    src/Devices/HomeDeviceBase.cpp
    src/Devices/SwitchableComponent.cpp
    src/Devices/DimmableComponent.cpp
    src/Devices/ColorableComponent.cpp
    src/Devices/StoppableComponent.cpp
    src/Devices/StatusComponent.cpp
    src/Devices/JsonSettingsManager.cpp
    include/Core/ThreadSafeValue.h
    include/UI/SensorBridge.h
    include/Network/HaWebSocketController.h
    include/Network/IHaController.h
    include/Devices/HomeDeviceBase.h
    include/Devices/IDeviceFactory.h
    include/Devices/DeviceFactory.h
    include/Devices/DeviceModel.h
    include/Devices/IDeviceComponent.h
    include/Devices/SwitchableComponent.h
    include/Devices/DimmableComponent.h
    include/Devices/ColorableComponent.h
    include/Devices/StoppableComponent.h
    include/Devices/StatusComponent.h
    include/UI/HaImageProvider.h
    include/Devices/ISettingsManager.h
    include/Devices/JsonSettingsManager.h
)

qt_add_executable(HomeControl ${PROJECT_SOURCES})

qt_add_qml_module(HomeControl
    URI SensorsApp
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        main.qml
        NetworkStatusIndicator.qml
        NetworkDiagnosticsSheet.qml
        DevToolsConsole.qml
)

qt_add_translations(HomeControl
    TS_FILES
        i18n/HomeControl_es.ts
        i18n/HomeControl_de.ts
)

target_link_libraries(HomeControl
    PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::WebSockets Qt6::Network
)

# --- Security Hardening Compiler Flags ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(NOT ANDROID)
        # Security options for independent executables on Desktop (Linux/macOS Clang/GNU)
        target_compile_options(HomeControl PRIVATE
            -Wall -Wextra -Wpedantic
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
            -Wformat -Wformat-security
            -fPIE
        )
        target_link_options(HomeControl PRIVATE -pie -z noexecstack -z relro -z now)
    endif()
elseif(MSVC)
    target_compile_options(HomeControl PRIVATE
        /W4
        /GS
        /sdl
        /guard:cf
    )
    target_link_options(HomeControl PRIVATE /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA)
endif()

if(ANDROID)
    set_property(TARGET HomeControl PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_property(TARGET HomeControl PROPERTY QT_ANDROID_COMPILE_SDK_VERSION "36")
    set_property(TARGET HomeControl PROPERTY QT_ANDROID_TARGET_SDK_VERSION "36")
    set_property(TARGET HomeControl PROPERTY QT_ANDROID_MIN_SDK_VERSION "28")
    set_property(TARGET HomeControl PROPERTY QT_ANDROID_PACKAGE_NAME "com.danielmontes.homecontrol")
endif()

# --- Unit Testing (GoogleTest) -
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
    tests/SensorBridgeTest.cpp
    tests/JsonSettingsManagerTest.cpp
    tests/DeviceFactoryTest.cpp
    tests/DeviceModelTest.cpp
    tests/GenericHaCommandTest.cpp
    tests/HaWebSocketControllerTest.cpp
    tests/DeviceComponentTest.cpp
    ${LIB_SOURCES}
)

target_include_directories(HomeControlTests PRIVATE 
    include
    include/Core
    include/Devices
    include/Mqtt
    include/UI
)

target_link_libraries(HomeControlTests
    PRIVATE
    GTest::gtest_main
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::WebSockets Qt6::Network
)

include(GoogleTest)
gtest_discover_tests(HomeControlTests)
