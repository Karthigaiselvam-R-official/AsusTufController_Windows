# AsusTufFanControl (Windows Port)

A native Windows port of the [AsusTufFanControl](https://github.com/flukejones/asus-fan-control) tool for Linux. This application provides advanced control over fan speeds, system styling (Aura Sync), and battery charging limits for ASUS TUF Gaming laptops.

> **Note:** This project is currently in active development.

## Features

-   **Fan Control**: 
    -   Silent, Balanced, and Turbo presets.
    -   **100% Fan Speed Mode** (approx. 6000 RPM) using `AsusWinIO` driver injection (similar to G-Helper).
    -   Custom Fan Curves (Coming Soon).
-   **Aura Sync**:
    -   Control RGB keyboard backlighting (Static, Breathing, Strobing, Rainbow).
    -   Adjust speed, brightness, and color.
-   **Battery Health Charging**:
    -   Set charge limit (e.g., 60%, 80%) to prolong battery lifespan.
-   **System Monitoring**:
    -   Real-time CPU & GPU usage and temperature.
    -   Fan RPM monitoring.
    -   Disk and Network usage statistics.
-   **UI**:
    -   Modern, hardware-accelerated QML user interface.
    -   Dark/Light mode support.

## Requirements

-   Windows 10 or Windows 11.
-   **Administrator Privileges** (Required for WMI and Hardware Access).
-   ASUS System Control Interface driver (installed by default on ASUS laptops).

## Installation / Building

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/Karthigaiselvam-R-official/AsusTufController_Windows.git
    ```
2.  **Dependencies**:
    -   Qt 6.6+ (MSVC 2019 64-bit or newer)
    -   Visual Studio 2022 (C++ Desktop Development workload)
    -   CMake 3.16+
3.  **Build**:
    Open the project in Qt Creator or use CMake from the terminal:
    ```bash
    mkdir build
    cd build
    cmake -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64" ..
    cmake --build . --config Release
    ```
4.  **Run**:
    Execute `AsusTufFanControl_Windows.exe` as Administrator.

## License

This project is licensed under the same license as the original Linux project (GPL-3.0). See the [LICENSE](LICENSE) file for details.

## Credits

-   **Original Linux Project**: [flukejones/asus-fan-control](https://github.com/flukejones/asus-fan-control)
-   **Windows Port Author**: Karthigaiselvam R
