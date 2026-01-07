#include "FanController.h"
#include "platform/windows/AsusWinIO.h"
#include "platform/windows/WmiWrapper.h"
#include <QDebug>
#include <algorithm>

FanController::FanController(QObject *parent)
    : QObject(parent)
    , m_cpuFanRpm(0)
    , m_gpuFanRpm(0)
    , m_cpuTemp(0)
    , m_gpuTemp(0)
    , m_isManualModeActive(false)
    , m_statusMessage("Ready")
{
    // Initialize Hardware Interfaces
    bool winIoSuccess = AsusWinIO::instance().initialize();
    if (winIoSuccess) {
        qDebug() << "AsusWinIO initialized successfully";
    } else {
        qCritical() << "Failed to initialize AsusWinIO - 6000RPM mode will not work";
        setStatusMessage("Error: WinIO Driver Failed");
    }

    WmiWrapper::instance().initialize(); // Attempt WMI init

    // Start Stats Timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &FanController::updateStats);
    m_updateTimer->start(1000); // 1 second interval
}

void FanController::setFanSpeed(int percentage)
{
    if (!m_isManualModeActive) {
        m_isManualModeActive = true;
        emit isManualModeActiveChanged();
    }
    
    applyFanSpeed(percentage);
    
    QString msg = QString("Manual: %1%").arg(percentage);
    if (percentage > 99) msg += " (OVERDRIVE)";
    setStatusMessage(msg);
}

void FanController::enableAutoMode()
{
    m_isManualModeActive = false;
    emit isManualModeActiveChanged();
    
    // Disable Test Mode (IMPORTANT)
    AsusWinIO::instance().setFanTestMode(false);
    
    // Set WMI Policy to Balanced (0) or similar default
    // ID 0x00110019 is often Thermal Policy
    // 0 = Balanced, 1 = Turbo, 2 = Silent (Check mapping)
    WmiWrapper::instance().setDevice(0x00110019, 0); 
    
    setStatusMessage("Auto Mode Active");
}

void FanController::applyFanSpeed(int percentage)
{
    // Hybrid Control Strategy
    
    if (percentage >= 99) {
        // OVERDRIVE MODE (6000 RPM)
        // Use AsusWinIO to force max PWM
        AsusWinIO::instance().setFanTestMode(true);
        AsusWinIO::instance().setFanPwmDuty(255); // Max PWM
    } else {
        // NORMAL FW CONTROL or MANUAL PWM
        // Ideally, we want to set a custom curve, but WMI usually only allows presets.
        // If we want manual PWM control for < 100%, we must use Test Mode.
        // "Test Mode" allows direct PWM writing.
        
        AsusWinIO::instance().setFanTestMode(true);
        
        // Map 0-100 to 0-255
        unsigned char pwm = static_cast<unsigned char>((percentage / 100.0) * 255);
        AsusWinIO::instance().setFanPwmDuty(pwm);
    }
}

void FanController::updateStats()
{
    // READ SENSORS
    // This is tricky on Windows without a specific driver wrapper for sensors.
    // AsusWinIO might expose reading RPM?
    // The C# code had "GetFanSpeed"?
    // I need to check if AsusWinIO library exposes "HealthyTable_FanReadRPM" or similar?
    // The C# file I viewed `AsusControl.cs` had `GetFanSpeed`.
    // It used `AsusWinIO64.HealthyTable_FanRPM`.
    
    // I need to update AsusWinIO wrapper to include reading if possible.
    // For now, I will use placeholder values or WMI if available.
    // WMI ID 0x00110013/14 are for Fan Control/Status?
    
    // For prototype:
    // We can't easily read temps without WMI ThermalZone or WinRing0.
    // WMI `MSAcpi_ThermalZoneTemperature` is standard.
    // I will try to implement reading in WmiWrapper later.
    
    // Placeholder logic for UI testing:
    // Simulate changes
    // m_cpuTemp = 45 + (m_cpuFanRpm > 0 ? 0 : 5); // Dummy
    // emit cpuTempChanged();
    
    // TODO: Implement actual reading
}

void FanController::setStatusMessage(const QString &message)
{
    if (m_statusMessage != message) {
        m_statusMessage = message;
        emit statusMessageChanged();
    }
}
