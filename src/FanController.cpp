#include "FanController.h"
#include "platform/windows/AsusWinIO.h"
#include "platform/windows/WmiWrapper.h"
#include <QDebug>
#include <QObject>
#include <QString>
#include <QTimer>
#include <algorithm>
#include <windows.h> // For Sleep

#define qsTr tr

FanController::FanController(QObject *parent)
    : QObject(parent), m_cpuFanRpm(0), m_gpuFanRpm(0), m_cpuTemp(0),
      m_gpuTemp(0), m_isManualModeActive(false), m_statusMessage("Ready"),
      m_fanCount(2), m_targetFanSpeed(0) {
  // Initialize Hardware Interfaces
  bool winIoSuccess = AsusWinIO::instance().initialize();
  if (winIoSuccess) {
    // Get actual fan count from driver
    m_fanCount = AsusWinIO::instance().getFanCounts();
    if (m_fanCount <= 0) {
      m_fanCount = 2; // Default to 2 fans
    }
  } else {
    setStatusMessage(qsTr("Error: WinIO Driver Failed"));
  }

  WmiWrapper::instance().initialize(); // Attempt WMI init

  // Start Stats Timer
  m_updateTimer = new QTimer(this);
  connect(m_updateTimer, &QTimer::timeout, this, &FanController::updateStats);
  m_updateTimer->start(1000); // 1 second interval

  // Initialize Ramp Timer
  m_rampTimer = new QTimer(this);
  m_rampTimer->setInterval(50); // 50ms updates (Standard responsiveness)
  connect(m_rampTimer, &QTimer::timeout, this, &FanController::processFanRamp);
  m_currentRampSpeed = 0;
}

void FanController::startRamp(int target) {
  if (m_rampTimer->isActive()) {
    // Update target on the fly if already ramping
  } else {
    // Start ramping from current KNOWN speed
    m_rampTimer->start();
  }
}

void FanController::processFanRamp() {
  double target = static_cast<double>(m_targetFanSpeed);
  double step = 5.0; // 5% per 50ms = 100% per second (1 sec full sweep)

  if (m_currentRampSpeed < target) {
    m_currentRampSpeed += step;
    if (m_currentRampSpeed > target)
      m_currentRampSpeed = target;
  } else if (m_currentRampSpeed > target) {
    m_currentRampSpeed -= step;
    if (m_currentRampSpeed < target)
      m_currentRampSpeed = target;
  }

  // Apply the intermediate speed
  applyFanSpeed(static_cast<int>(m_currentRampSpeed));

  // Emit signal for smooth UI (updates every 50ms)
  emit currentRampSpeedChanged();

  // Stop if reached
  if (abs(m_currentRampSpeed - target) < 0.1) {
    m_rampTimer->stop();
    m_currentRampSpeed = target;
    applyFanSpeed(static_cast<int>(target)); // Final strict apply
    emit currentRampSpeedChanged();          // Ensure final value is sent
  }
}

void FanController::setFanSpeed(int percentage) {
  if (!m_isManualModeActive) {
    m_isManualModeActive = true;
    emit isManualModeActiveChanged();
  }

  m_targetFanSpeed = percentage;
  startRamp(percentage); // Use Ramp

  QString msg = qsTr("Manual: %1%").arg(percentage);
  if (percentage > 99)
    msg += qsTr(" (OVERDRIVE)");
  setStatusMessage(msg);
}

void FanController::enableAutoMode() {
  m_isManualModeActive = false;
  emit isManualModeActiveChanged();
  m_rampTimer->stop(); // Stop any manual ramping

  // Disable Test Mode (IMPORTANT)
  AsusWinIO::instance().setFanTestMode(false);

  // Set WMI Policy to Balanced (0) or similar default
  // ID 0x00110019 is often Thermal Policy
  // 0 = Balanced, 1 = Turbo, 2 = Silent (Check mapping)
  WmiWrapper::instance().setDevice(0x00110019, 0);

  setStatusMessage(qsTr("Auto Mode Active"));
}

void FanController::enableManualModeWithSync() {
  // 1. Calculate current max percentage from RPMs
  // Assuming 6000 RPM is roughly 100%
  int cpuPercent = static_cast<int>((m_cpuFanRpm / 6000.0) * 100);
  int gpuPercent = static_cast<int>((m_gpuFanRpm / 6000.0) * 100);

  // Safety clamp
  cpuPercent = std::clamp(cpuPercent, 0, 100);
  gpuPercent = std::clamp(gpuPercent, 0, 100);

  // Pick the higher one for safety
  int safeStartPercent = (cpuPercent > gpuPercent) ? cpuPercent : gpuPercent;

  // 2. Set the target speed (updates slider via property binding)
  // We do NOT call setFanSpeed yet to avoid double-ramp, but we need
  // to update m_targetFanSpeed so the slider jumps there.
  m_targetFanSpeed = safeStartPercent;
  // FORCE emit even if value is same (e.g. 0 -> 0) to ensure UI sync
  emit targetFanSpeedChanged();

  // 3. Enable Manual Mode
  if (!m_isManualModeActive) {
    m_isManualModeActive = true;
    emit isManualModeActiveChanged();
  }

  // 4. Start Ramp from this calculated point (Smooth handover)
  // Initialize ramp speed to current target to avoid "ramp up" from 0
  m_currentRampSpeed = safeStartPercent;
  startRamp(
      safeStartPercent); // Effectively holds current speed or minor adjustment

  setStatusMessage(qsTr("Manual: %1% (Synced)").arg(safeStartPercent));
}

void FanController::setAutoFanSpeed(int percentage) {
  // Ensure we are logically in Auto Mode
  m_isManualModeActive = false;
  emit isManualModeActiveChanged();

  m_targetFanSpeed = percentage;
  startRamp(percentage); // Use Ramp

  setStatusMessage(qsTr("Auto (Curve): %1%").arg(percentage));
}

void FanController::setThermalPolicy(int policy) {
  // 1. Try WMI Thermal Policy first (Best method)
  // 0 = Balanced, 1 = Turbo, 2 = Silent
  bool success = WmiWrapper::instance().setThermalPolicy(policy);

  QString modeName;
  if (policy == 2)
    modeName = qsTr("Silent");
  else if (policy == 0)
    modeName = qsTr("Balanced");
  else
    modeName = qsTr("Turbo");

  if (success) {

    setStatusMessage(qsTr("Auto: %1 Mode").arg(modeName));
    m_rampTimer->stop(); // Ensure manual ramp is stopped
    return;
  }

  // 2. Fallback: If WMI fails, use Manual Fan Control
  qWarning() << "WMI Policy failed, falling back to Manual control";

  int fanPercent = 0;
  if (policy == 2)
    fanPercent = 0; // Silent (0% - True Silence)
  else if (policy == 0)
    fanPercent = 40; // Balanced (Moderate ~2500-3000 RPM)
  else
    fanPercent = 95; // Turbo (Max Performance)

  // Set the manual speed but keep "Auto" flag logically
  // (We use applyFanSpeed directly to avoid setting m_isManualModeActive =
  // true)
  m_targetFanSpeed = fanPercent;
  startRamp(fanPercent); // Use Ramp for smooth fallback transition

  setStatusMessage(qsTr("Auto (Fallback): %1%").arg(fanPercent));
}

void FanController::applyFanSpeed(int percentage) {
  // Map 0-100% to 0-255 PWM
  unsigned char pwm = static_cast<unsigned char>((percentage / 100.0) * 255);

  // Clamp to max for overdrive
  if (percentage >= 99) {
    pwm = 255;
  }

  // Ensure Manual Mode is ON globally first
  AsusWinIO::instance().setFanTestMode(true);
  Sleep(50); // Give EC time to switch modes

  // Apply to ALL fans (CPU fan = index 0, GPU fan = index 1, etc.)
  for (int i = 0; i < m_fanCount; i++) {
    AsusWinIO::instance().setFanIndex(static_cast<unsigned char>(i));
    Sleep(20); // Wait for index switch
    AsusWinIO::instance().setFanPwmDuty(pwm);
    Sleep(20); // Wait for PWM write
  }
}

void FanController::updateStats() {
  // Re-apply Fan Speed in Manual Mode to prevent EC Watchdog reset
  // BUT only if we are not currently ramping (ramp handles updates)
  if (m_isManualModeActive && !m_rampTimer->isActive()) {
    applyFanSpeed(m_targetFanSpeed);
  }

  // READ SENSORS FROM ASUSWINIO DRIVER

  // Read CPU Temperature
  int newCpuTemp = AsusWinIO::instance().getCpuTemp();
  if (newCpuTemp != m_cpuTemp) {
    m_cpuTemp = newCpuTemp;
    emit cpuTempChanged();
  }

  // Read CPU Fan RPM (index 0)
  int newCpuRpm = AsusWinIO::instance().getFanRPM(0);
  if (newCpuRpm < 0)
    newCpuRpm = 0;
  if (newCpuRpm != m_cpuFanRpm) {
    m_cpuFanRpm = newCpuRpm;
    emit cpuFanRpmChanged();
  }

  // Read GPU Fan RPM (index 1) if available
  if (m_fanCount > 1) {
    int newGpuRpm = AsusWinIO::instance().getFanRPM(1);
    if (newGpuRpm < 0)
      newGpuRpm = 0;
    if (newGpuRpm != m_gpuFanRpm) {
      m_gpuFanRpm = newGpuRpm;
      emit gpuFanRpmChanged();
    }
  }

  // GPU Temperature is harder - AsusWinIO may not expose it
  // Could use nvidia-smi for NVIDIA GPUs, but that's handled elsewhere
  // For now, leave gpuTemp unchanged (0 or populated by SystemStatsMonitor)
}

void FanController::setStatusMessage(const QString &message) {
  if (m_statusMessage != message) {
    m_statusMessage = message;
    emit statusMessageChanged();
  }
}
