#include "FanCurveController.h"
#include "FanController.h"
#include "platform/windows/AsusWinIO.h"
#include <QObject>

FanCurveController::FanCurveController(QObject *parent)
    : QObject(parent), m_fanController(nullptr), m_autoCurveEnabled(false),
      m_silentThreshold(55) // Default: Silent mode below 55°C
      ,
      m_balancedThreshold(75) // Default: Balanced between 55-75°C, Turbo above
      ,
      m_currentCpuTemp(0), m_currentPolicy("Idle"), m_lastAppliedPolicy(-1) {
  // Load saved thresholds
  loadSettings();

  // Create evaluation timer (every 2 seconds when enabled)
  m_evaluationTimer = new QTimer(this);
  m_evaluationTimer->setInterval(2000);
  connect(m_evaluationTimer, &QTimer::timeout, this,
          &FanCurveController::evaluateTemperature);
}

void FanCurveController::setFanController(FanController *controller) {
  if (m_fanController != controller) {
    m_fanController = controller;
    emit fanControllerChanged();
  }
}

void FanCurveController::setAutoCurveEnabled(bool enabled) {
  if (m_autoCurveEnabled != enabled) {
    m_autoCurveEnabled = enabled;
    emit autoCurveEnabledChanged();

    if (enabled) {

      // Start evaluating temperatures
      m_evaluationTimer->start();
      // Do an immediate evaluation
      evaluateTemperature();
    } else {

      // Stop evaluating
      m_evaluationTimer->stop();
      m_lastAppliedPolicy = -1;

      // Return control to the FanController's auto mode
      if (m_fanController) {
        m_fanController->enableAutoMode();
      }
    }
  }
}

void FanCurveController::setSilentThreshold(int temp) {
  if (m_silentThreshold != temp) {
    m_silentThreshold = temp;
    emit thresholdsChanged();
    saveSettings();
  }
}

void FanCurveController::setBalancedThreshold(int temp) {
  if (m_balancedThreshold != temp) {
    m_balancedThreshold = temp;
    emit thresholdsChanged();
    saveSettings();
  }
}

void FanCurveController::evaluateTemperature() {
  // Read current CPU temperature from the driver
  m_currentCpuTemp = AsusWinIO::instance().getCpuTemp();
  emit currentCpuTempChanged();

  // Determine target fan speed based on linear interpolation
  // Logic:
  // < SilentThreshold = 0%
  // > BalancedThreshold = 100%
  // In-between = Linear scale from 30% to 100%

  int targetSpeed = 0;

  if (m_currentCpuTemp <= m_silentThreshold) {
    targetSpeed = 0; // Silent
  } else if (m_currentCpuTemp >= m_balancedThreshold) {
    targetSpeed = 100; // Turbo/Max
  } else {
    // Linear Interpolation
    double range = m_balancedThreshold - m_silentThreshold;
    double offset = m_currentCpuTemp - m_silentThreshold;
    double ratio = offset / range;

    // Scale from 30% to 100%
    targetSpeed = static_cast<int>(30 + (70.0 * ratio));
  }

  // Only apply if speed changed significantly (hysteresis of 2%)
  // or if we are just starting (lastSpeed == -1)
  static int lastSpeed = -1;
  if (abs(targetSpeed - lastSpeed) >= 2 || targetSpeed == 0 ||
      targetSpeed == 100 || lastSpeed == -1) {
    if (m_fanController) {
      m_fanController->setAutoFanSpeed(targetSpeed);
      lastSpeed = targetSpeed;
    }
  }

  // Update UI policy name for display
  QString policyName;
  if (targetSpeed == 0)
    policyName = "Silent";
  else if (targetSpeed == 100)
    policyName = "Turbo";
  else
    policyName = "Balanced";

  if (m_currentPolicy != policyName) {
    m_currentPolicy = policyName;
    emit currentPolicyChanged();
  }
}

void FanCurveController::applyPolicy(int policy) {
  // Deprecated - Logic moved to evaluateTemperature
  // But for safety, if called externally:
  if (m_fanController) {
    m_fanController->setThermalPolicy(policy);
  }
}

void FanCurveController::applyPreset(const QString &presetName) {
  if (presetName == "Gaming") {
    m_silentThreshold = 40;
    m_balancedThreshold = 60;
  } else if (presetName == "Quiet") {
    m_silentThreshold = 65;
    m_balancedThreshold = 80;
  } else if (presetName == "Balanced") {
    m_silentThreshold = 50;
    m_balancedThreshold = 70;
  } else if (presetName == "Performance") {
    m_silentThreshold = 35;
    m_balancedThreshold = 50;
  }

  saveSettings();
  emit thresholdsChanged();

  if (m_currentPreset != presetName) {
    m_currentPreset = presetName;
    emit currentPresetChanged();
  }

  // Force re-evaluation
  if (m_autoCurveEnabled) {
    evaluateTemperature();
  }
}

void FanCurveController::loadSettings() {
  QSettings settings("AsusTuf", "FanControl");
  m_silentThreshold = settings.value("FanCurve/SilentThreshold", 55).toInt();
  m_balancedThreshold =
      settings.value("FanCurve/BalancedThreshold", 75).toInt();
  m_currentPreset = settings.value("FanCurve/CurrentPreset", "").toString();
}

void FanCurveController::saveSettings() {
  QSettings settings("AsusTuf", "FanControl");
  settings.setValue("FanCurve/SilentThreshold", m_silentThreshold);
  settings.setValue("FanCurve/BalancedThreshold", m_balancedThreshold);
  settings.setValue("FanCurve/CurrentPreset", m_currentPreset);
}
