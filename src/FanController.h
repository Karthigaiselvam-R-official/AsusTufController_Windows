#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QString>
#include <QTimer>

class FanController : public QObject {
  Q_OBJECT
  Q_PROPERTY(int cpuFanRpm READ cpuFanRpm NOTIFY cpuFanRpmChanged)
  Q_PROPERTY(int gpuFanRpm READ gpuFanRpm NOTIFY gpuFanRpmChanged)
  Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
  Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
  Q_PROPERTY(bool isManualModeActive READ isManualModeActive NOTIFY
                 isManualModeActiveChanged)
  Q_PROPERTY(
      QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(double currentRampSpeed READ currentRampSpeed NOTIFY
                 currentRampSpeedChanged)
  Q_PROPERTY(int targetFanSpeed READ targetFanSpeed NOTIFY
                 targetFanSpeedChanged) // FIXED: Expose targetFanSpeed

public:
  explicit FanController(QObject *parent = nullptr);

  // Properties interaction
  int cpuFanRpm() const { return m_cpuFanRpm; }
  int gpuFanRpm() const { return m_gpuFanRpm; }
  int cpuTemp() const { return m_cpuTemp; }
  int gpuTemp() const { return m_gpuTemp; }
  bool isManualModeActive() const { return m_isManualModeActive; }
  QString statusMessage() const { return m_statusMessage; }
  double currentRampSpeed() const { return m_currentRampSpeed; }
  int targetFanSpeed() const {
    return m_targetFanSpeed;
  } // Getter for Q_PROPERTY

  // Invokables for QML
  Q_INVOKABLE void setFanSpeed(int percentage);
  Q_INVOKABLE void enableAutoMode();
  Q_INVOKABLE void
  enableManualModeWithSync(); // New: Syncs slider to current speed
  Q_INVOKABLE void setThermalPolicy(int policy);
  Q_INVOKABLE void setAutoFanSpeed(int percentage); // New: For dynamic curves

signals:
  void cpuFanRpmChanged();
  void gpuFanRpmChanged();
  void cpuTempChanged();
  void gpuTempChanged();
  void targetFanSpeedChanged(); // Missing signal
  void isManualModeActiveChanged();
  void statusMessageChanged();
  void currentRampSpeedChanged(); // Signal for smooth UI
  void statsUpdated();            // Signal for QML to know stats were refreshed

private slots:
  void updateStats();

private:
  void setStatusMessage(const QString &message);
  void applyFanSpeed(int percentage);

  int m_cpuFanRpm;
  int m_gpuFanRpm;
  int m_cpuTemp;
  int m_gpuTemp;
  bool m_isManualModeActive;
  QString m_statusMessage;
  int m_fanCount; // Number of controllable fans (from driver)
  int m_targetFanSpeed;

  QTimer *m_updateTimer;

  // Ramping Mechanics
  QTimer *m_rampTimer;
  double m_currentRampSpeed; // Use double for smooth increments
  void startRamp(int target);

private slots:
  void processFanRamp();
};

#endif // FANCONTROLLER_H
