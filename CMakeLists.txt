cmake_minimum_required(VERSION 3.16)

project(AsusTufController_Windows VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Quick Widgets QuickControls2 LinguistTools)

qt_standard_project_setup()

# Translation files
set(TS_FILES
    translations/AsusTufController_ar.ts
    translations/AsusTufController_bn.ts
    translations/AsusTufController_de.ts
    translations/AsusTufController_es.ts
    translations/AsusTufController_fa.ts
    translations/AsusTufController_fr.ts
    translations/AsusTufController_hi.ts
    translations/AsusTufController_id.ts
    translations/AsusTufController_it.ts
    translations/AsusTufController_ja.ts
    translations/AsusTufController_ko.ts
    translations/AsusTufController_mr.ts
    translations/AsusTufController_pa.ts
    translations/AsusTufController_pl.ts
    translations/AsusTufController_pt.ts
    translations/AsusTufController_ru.ts
    translations/AsusTufController_sw.ts
    translations/AsusTufController_ta.ts
    translations/AsusTufController_tr.ts
    translations/AsusTufController_ur.ts
    translations/AsusTufController_vi.ts
    translations/AsusTufController_zh.ts
)

set(PROJECT_SOURCES
    src/main.cpp
    src/FanController.h src/FanController.cpp
    src/AuraController.h src/AuraController.cpp
    src/SystemStatsMonitor.h src/SystemStatsMonitor.cpp
    src/FanCurveController.h src/FanCurveController.cpp
    src/FanCurveController.h src/FanCurveController.cpp
    src/LanguageController.h src/LanguageController.cpp
    src/ThemeController.h src/ThemeController.cpp
    src/platform/windows/AsusWinIO.h src/platform/windows/AsusWinIO.cpp
    src/platform/windows/WmiWrapper.h src/platform/windows/WmiWrapper.cpp
    src/platform/windows/AuraHID.h src/platform/windows/AuraHID.cpp
    src/platform/windows/AtkAcpi.h src/platform/windows/AtkAcpi.cpp
)

qt_add_executable(AsusTufController_Windows
    WIN32
    MANUAL_FINALIZATION
    src/app.rc
    ${PROJECT_SOURCES}
)

# QML and Resources
qt_add_qml_module(AsusTufController_Windows
    URI "AsusTufController"
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        ui/Main.qml
        ui/GraphCard.qml
        ui/StatsCard.qml
        ui/CircularGauge.qml
        ui/components/Sidebar.qml
        ui/pages/DashboardPage.qml
        ui/pages/FanPage.qml
        ui/pages/AuraPage.qml
        ui/pages/BatteryPage.qml
        ui/pages/SettingsPage.qml
        ui/Translations.js
    RESOURCES
        resources/FanControl.png
        resources/AuraSync.png
        resources/BatteryManagement.png
        resources/SystemInfo.png
        ui/app_icon.png
)

# Compile translations (.ts -> .qm) and embed them
qt_add_translations(AsusTufController_Windows
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/translations"
)

# Libs
target_link_libraries(AsusTufController_Windows
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    wbemuuid # WMI
    pdh      # Performance Counters
    iphlpapi # Network Stats
)

# Copy the DLL to the build directory
add_custom_command(TARGET AsusTufController_Windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/AsusWinIO64.dll"
    "$<TARGET_FILE_DIR:AsusTufController_Windows>/AsusWinIO64.dll"
)

# Copy PsExec.exe and run.bat to the build directory
add_custom_command(TARGET AsusTufController_Windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/PsExec.exe"
    "$<TARGET_FILE_DIR:AsusTufController_Windows>/PsExec.exe"
)
add_custom_command(TARGET AsusTufController_Windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/run.bat"
    "$<TARGET_FILE_DIR:AsusTufController_Windows>/run.bat"
)

add_custom_command(TARGET AsusTufController_Windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:AsusTufController_Windows>/resources"
)

install(TARGETS AsusTufController_Windows
    BUNDLE DESTINATION .
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
