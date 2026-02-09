#ifndef THEMECONTROLLER_H
#define THEMECONTROLLER_H

#include <QObject>
#include <QSettings>

class ThemeController : public QObject {
  Q_OBJECT
  Q_PROPERTY(ThemeMode themeMode READ themeMode WRITE setThemeMode NOTIFY
                 themeModeChanged)
  Q_PROPERTY(bool isDark READ isDark NOTIFY isDarkChanged)

public:
  enum ThemeMode { System = 0, Light = 1, Dark = 2 };
  Q_ENUM(ThemeMode)

  enum TempUnit { Celsius = 0, Fahrenheit = 1 };
  Q_ENUM(TempUnit)
  Q_PROPERTY(
      TempUnit tempUnit READ tempUnit WRITE setTempUnit NOTIFY tempUnitChanged)

  explicit ThemeController(QObject *parent = nullptr);

  ThemeMode themeMode() const;
  bool isDark() const;
  TempUnit tempUnit() const;

  Q_INVOKABLE void setThemeMode(ThemeMode mode);
  Q_INVOKABLE void toggle();
  Q_INVOKABLE void setTempUnit(TempUnit unit);
  Q_INVOKABLE int toFahrenheit(int celsius) const {
    return (celsius * 9 / 5) + 32;
  }

signals:
  void themeModeChanged();
  void isDarkChanged();
  void tempUnitChanged();

private:
  void checkSystemTheme();
#ifdef Q_OS_WIN
  bool readDarkModeFromRegistry() const;
#endif

  ThemeMode m_themeMode;
  TempUnit m_tempUnit;
  bool m_isDark;
  QSettings m_settings;
  QTimer *m_timer;
};

#endif // THEMECONTROLLER_H