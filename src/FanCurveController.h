#ifndef FANCURVECONTROLLER_H
#define FANCURVECONTROLLER_H

#include <QObject>
#include <QSettings>
#include <QTimer>

class FanController; // Forward declaration

class FanCurveController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool autoCurveEnabled READ autoCurveEnabled WRITE
                 setAutoCurveEnabled NOTIFY autoCurveEnabledChanged)
  Q_PROPERTY(int silentThreshold READ silentThreshold WRITE setSilentThreshold
                 NOTIFY thresholdsChanged)
  Q_PROPERTY(int balancedThreshold READ balancedThreshold WRITE
                 setBalancedThreshold NOTIFY thresholdsChanged)
  Q_PROPERTY(
      int currentCpuTemp READ currentCpuTemp NOTIFY currentCpuTempChanged)
  Q_PROPERTY(
      QString currentPolicy READ currentPolicy NOTIFY currentPolicyChanged)
  Q_PROPERTY(FanController *fanController READ fanController WRITE
                 setFanController NOTIFY fanControllerChanged)

public:
  explicit FanCurveController(QObject *parent = nullptr);

  bool autoCurveEnabled() const { return m_autoCurveEnabled; }
  int silentThreshold() const { return m_silentThreshold; }
  int balancedThreshold() const { return m_balancedThreshold; }
  int currentCpuTemp() const { return m_currentCpuTemp; }
  QString currentPolicy() const { return m_currentPolicy; }
  FanController *fanController() const { return m_fanController; }

  void setAutoCurveEnabled(bool enabled);
  void setSilentThreshold(int temp);
  void setBalancedThreshold(int temp);
  void setFanController(FanController *controller);

  Q_INVOKABLE void applyPreset(const QString &presetName);

signals:
  void autoCurveEnabledChanged();
  void thresholdsChanged();
  void currentCpuTempChanged();
  void currentPolicyChanged();
  void fanControllerChanged();

private slots:
  void evaluateTemperature();

private:
  void loadSettings();
  void saveSettings();
  void applyPolicy(int policy);

  FanController *m_fanController;
  QTimer *m_evaluationTimer;

  bool m_autoCurveEnabled;
  int m_silentThreshold;   // Below this = Silent (fan ~20%)
  int m_balancedThreshold; // Above silent, below this = Balanced (~50%)
                           // Above balanced = Turbo (100%)
  int m_currentCpuTemp;
  QString m_currentPolicy;
  int m_lastAppliedPolicy;
};

#endif // FANCURVECONTROLLER_H
