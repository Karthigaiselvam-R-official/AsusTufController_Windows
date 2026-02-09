#include "ThemeController.h"
#include <QDebug>
#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>
#include <QTimer>

#ifdef Q_OS_WIN
#include "platform/windows/AtkAcpi.h"
#include <windows.h>
#endif

ThemeController::ThemeController(QObject *parent)
    : QObject(parent), m_themeMode(System), m_tempUnit(Celsius),
      m_isDark(false), // Start with false (Light mode)
      m_settings("AsusTuf", "FanControl") {

  // DEBUG: Check Aura Hardware State
  AtkAcpi::instance().initialize();
  // Simplified debug check
  AtkAcpi::instance().getTufKeyboardState();

  // Load saved preference
  int savedMode =
      m_settings.value("Appearance/ThemeMode", static_cast<int>(System))
          .toInt();
  m_themeMode = static_cast<ThemeMode>(savedMode);

  int savedUnit =
      m_settings.value("Appearance/TempUnit", static_cast<int>(Celsius))
          .toInt();
  m_tempUnit = static_cast<TempUnit>(savedUnit);

  // Initial system theme detection
  checkSystemTheme();

  // Listen for system theme changes
  connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
          &ThemeController::checkSystemTheme);

  // Auto-refresh timer for SYSTEM context (1000ms)
  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, &ThemeController::checkSystemTheme);
  m_timer->start(1000);
}

ThemeController::ThemeMode ThemeController::themeMode() const {
  return m_themeMode;
}

bool ThemeController::isDark() const { return m_isDark; }

void ThemeController::setThemeMode(ThemeMode mode) {
  if (m_themeMode == mode) {
    // Force check if System mode is re-selected
    if (mode == System) {
      checkSystemTheme();
    }
    return;
  }

  m_themeMode = mode;

  // Save preference
  m_settings.setValue("Appearance/ThemeMode", static_cast<int>(mode));

  emit themeModeChanged();
  checkSystemTheme(); // Apply immediately
}

void ThemeController::toggle() {
  ThemeMode nextMode;
  switch (m_themeMode) {
  case System:
    nextMode = Dark;
    break;
  case Dark:
    nextMode = Light;
    break;
  case Light:
    nextMode = System;
    break;
  default:
    nextMode = System;
    break;
  }
  setThemeMode(nextMode);
}

#ifdef Q_OS_WIN
#include <Lmcons.h> // For UNLEN
bool ThemeController::readDarkModeFromRegistry() const {
  // 1. Try HKEY_CURRENT_USER (Standard method)
  HKEY hKey;
  LPCWSTR subKey =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  DWORD value = 1;
  DWORD dataSize = sizeof(DWORD);

  bool isDark = false;
  bool foundInHKCU = false;

  LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0,
                              KEY_READ | KEY_WOW64_64KEY, &hKey);
  if (result == ERROR_SUCCESS) {
    if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                         reinterpret_cast<LPBYTE>(&value),
                         &dataSize) == ERROR_SUCCESS) {
      isDark = (value == 0);
      foundInHKCU = true;
    } else if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", nullptr, nullptr,
                                reinterpret_cast<LPBYTE>(&value),
                                &dataSize) == ERROR_SUCCESS) {
      isDark = (value == 0);
      foundInHKCU = true;
    }
    RegCloseKey(hKey);
  }

  // If found in HKCU and it says Dark, accept it.
  if (foundInHKCU && isDark) {
    return true;
  }

  // 2. Fallback: Iterate HKEY_USERS to find the actual logged-in user (for
  // SYSTEM context)
  HKEY hKeyUsers;
  if (RegOpenKeyExW(HKEY_USERS, nullptr, 0, KEY_READ | KEY_WOW64_64KEY,
                    &hKeyUsers) != ERROR_SUCCESS) {
    return foundInHKCU ? isDark : false;
  }

  WCHAR subkeyName[256];
  DWORD index = 0;
  DWORD subkeyNameLen;

  while (true) {
    subkeyNameLen = 256;
    LONG enumRes = RegEnumKeyExW(hKeyUsers, index, subkeyName, &subkeyNameLen,
                                 nullptr, nullptr, nullptr, nullptr);
    if (enumRes != ERROR_SUCCESS)
      break;

    QString qSubkeyName = QString::fromWCharArray(subkeyName);
    index++;

    // Filter for standard user SIDs (starts with S-1-5-21) and ignore _Classes
    if (!qSubkeyName.startsWith("S-1-5-21") ||
        qSubkeyName.endsWith("_Classes")) {
      continue;
    }

    // Construct full path for this user
    QString userPath =
        qSubkeyName +
        "\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

    HKEY hKeyUserTheme;
    if (RegOpenKeyExW(HKEY_USERS, (LPCWSTR)userPath.utf16(), 0,
                      KEY_READ | KEY_WOW64_64KEY,
                      &hKeyUserTheme) == ERROR_SUCCESS) {
      DWORD userValue = 1;
      DWORD userDataSize = sizeof(DWORD);

      if (RegQueryValueExW(hKeyUserTheme, L"AppsUseLightTheme", nullptr,
                           nullptr, reinterpret_cast<LPBYTE>(&userValue),
                           &userDataSize) == ERROR_SUCCESS) {
        bool userIsDark = (userValue == 0);
        RegCloseKey(hKeyUserTheme);
        RegCloseKey(hKeyUsers);
        return userIsDark; // Found a valid user setting, assume it's the one we
                           // want
      }
      RegCloseKey(hKeyUserTheme);
    }
  }

  RegCloseKey(hKeyUsers);

  // Default return
  return foundInHKCU ? isDark : false;
}
#endif

void ThemeController::checkSystemTheme() {
  bool newIsDark = false;

  if (m_themeMode == System) {
#ifdef Q_OS_WIN
    // On Windows, use Windows API
    newIsDark = readDarkModeFromRegistry();
#else
    // Non-Windows: Use Qt
    Qt::ColorScheme qtScheme = QGuiApplication::styleHints()->colorScheme();
    newIsDark = (qtScheme == Qt::ColorScheme::Dark);
#endif
  } else {
    // Manual mode
    newIsDark = (m_themeMode == Dark);
  }

  // Update if changed
  if (m_isDark != newIsDark) {
    m_isDark = newIsDark;
    emit isDarkChanged();
  }
}

ThemeController::TempUnit ThemeController::tempUnit() const {
  return m_tempUnit;
}

void ThemeController::setTempUnit(TempUnit unit) {
  if (m_tempUnit == unit)
    return;

  m_tempUnit = unit;
  m_settings.setValue("Appearance/TempUnit", static_cast<int>(unit));
  emit tempUnitChanged();
}