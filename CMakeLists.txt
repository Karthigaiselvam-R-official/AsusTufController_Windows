cmake_minimum_required(VERSION 3.16)

project(AsusTufFanControl_Windows VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Quick Widgets QuickControls2 LinguistTools)

qt_standard_project_setup()

# Translation files
set(TS_FILES
    translations/AsusTufFanControl_ar.ts
    translations/AsusTufFanControl_bn.ts
    translations/AsusTufFanControl_de.ts
    translations/AsusTufFanControl_es.ts
    translations/AsusTufFanControl_fa.ts
    translations/AsusTufFanControl_fr.ts
    translations/AsusTufFanControl_hi.ts
    translations/AsusTufFanControl_id.ts
    translations/AsusTufFanControl_it.ts
    translations/AsusTufFanControl_ja.ts
    translations/AsusTufFanControl_ko.ts
    translations/AsusTufFanControl_mr.ts
    translations/AsusTufFanControl_pa.ts
    translations/AsusTufFanControl_pl.ts
    translations/AsusTufFanControl_pt.ts
    translations/AsusTufFanControl_ru.ts
    translations/AsusTufFanControl_sw.ts
    translations/AsusTufFanControl_ta.ts
    translations/AsusTufFanControl_tr.ts
    translations/AsusTufFanControl_ur.ts
    translations/AsusTufFanControl_vi.ts
    translations/AsusTufFanControl_zh.ts
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

qt_add_executable(AsusTufFanControl_Windows
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# QML and Resources
qt_add_qml_module(AsusTufFanControl_Windows
    URI "AsusTufFanControl"
    VERSION 1.0
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
qt_add_translations(AsusTufFanControl_Windows
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/translations"
)

# Libs
target_link_libraries(AsusTufFanControl_Windows
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
add_custom_command(TARGET AsusTufFanControl_Windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/AsusWinIO64.dll"
    "$<TARGET_FILE_DIR:AsusTufFanControl_Windows>/AsusWinIO64.dll"
)

install(TARGETS AsusTufFanControl_Windows
    BUNDLE DESTINATION .
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
