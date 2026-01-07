#include "AuraController.h"
#include "platform/windows/WmiWrapper.h"
#include <QDebug>
#include <QColor>

// WMI Device IDs for TUF RGB (Based on Linux asus-wmi driver)
#define ASUS_WMI_DEVID_RGB_MODE  0x00100056
#define ASUS_WMI_DEVID_RGB_COLOR 0x00100057
#define ASUS_WMI_DEVID_RGB_SPEED 0x00100058 // Maybe? Or Cycle?

AuraController::AuraController(QObject *parent) 
    : QObject(parent)
{
    m_settings = new QSettings("AsusTufFanControl", "Aura", this);
}

void AuraController::setBrightness(int level)
{
    // 0x00100050 ?? Or standard Keyboard Backlight ID 0x00050021
    // Usually 0x00050021 is for keyboard brightness (0-3)
    WmiWrapper::instance().setDevice(0x00050021, level);
}

void AuraController::setStatic(QString color)
{
    setMode(0); // Static
    setColor(color);
}

void AuraController::setBreathing(QString color, int speed)
{
    setMode(1); // Breathing
    setColor(color);
    setSpeed(speed);
}

void AuraController::setRainbow(int speed)
{
    setMode(2); // Rainbow / Cycle
    setSpeed(speed);
}

void AuraController::setPulsing(QString color, int speed)
{
    setMode(3); // Strobing
    setColor(color);
    setSpeed(speed);
}

void AuraController::setMode(int mode)
{
    WmiWrapper::instance().setDevice(ASUS_WMI_DEVID_RGB_MODE, mode);
}

void AuraController::setColor(QString colorHex)
{
    if (colorHex.startsWith("#")) colorHex.remove(0, 1);
    
    QColor c("#" + colorHex);
    if (!c.isValid()) return;
    
    // Check format: usually R G B packed?
    // Often: 0x00RRGGBB or 0x01RRGGBB?
    // Linux driver writes: r << 16 | g << 8 | b
    
    DWORD colorVal = (c.red() << 16) | (c.green() << 8) | c.blue();
    WmiWrapper::instance().setDevice(ASUS_WMI_DEVID_RGB_COLOR, colorVal);
}

void AuraController::setSpeed(int speed)
{
    // Speed mapping might need adjustment
    WmiWrapper::instance().setDevice(ASUS_WMI_DEVID_RGB_SPEED, speed);
}

void AuraController::saveState(QString mode, QString color)
{
    m_settings->setValue("mode", mode);
    m_settings->setValue("color", color);
}

int AuraController::getSystemBrightness()
{
    // Read not implemented in WmiWrapper yet
    return 3; 
}

QString AuraController::getLastMode()
{
    return m_settings->value("mode", "Static").toString();
}

QString AuraController::getLastColor()
{
    return m_settings->value("color", "FF0000").toString();
}
