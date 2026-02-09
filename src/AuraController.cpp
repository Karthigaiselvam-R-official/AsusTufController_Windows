#include "AuraController.h"
#include "platform/windows/AtkAcpi.h"
#include <QColor>

AuraController::AuraController(QObject *parent) : QObject(parent) {
  m_settings = new QSettings("AsusTufFanControl", "Aura", this);
  m_strobeTimer = new QTimer(this);
  connect(m_strobeTimer, &QTimer::timeout, this,
          &AuraController::onStrobeTimeout);
  m_strobeToggle = false;
  m_strobeColor = "FF0000";

  // Initialize tracking variables
  m_lastReadMode = "";
  m_lastReadBrightness = -1;
  m_initializedFromSettings = false; // Start as false

  AtkAcpi::instance().initialize();
  initFromHardware();

  // Start Polling Timer (1000ms)
  m_pollTimer = new QTimer(this);
  connect(m_pollTimer, &QTimer::timeout, this, &AuraController::onPollTimeout);
  m_pollTimer->start(1000);
}

void AuraController::onPollTimeout() { initFromHardware(); }

void AuraController::initFromHardware() {
  // 1. Startup Logic: Restore from Settings (Ignore Hardware Mode initially)
  if (!m_initializedFromSettings) {
    QString savedMode = getLastMode();
    QString savedColor = getLastColor();
    int savedSpeed = getLastSpeed();

    // Apply the saved state to hardware to ensure synchronization
    applyMode(savedMode, savedColor, savedSpeed);

    // Mark as initialized so we don't force it again
    m_initializedFromSettings = true;

    // Sync internal state to match what we just applied
    m_lastReadMode = savedMode;
    return;
  }

  // 2. Polling Logic: Check for Hardware Changes
  auto state = AtkAcpi::instance().getTufKeyboardState();
  if (state.isValid) {
    // Sync Mode
    QString modeStr = "Static";
    switch (state.mode) {
    case TUF_MODE_STATIC:
      modeStr = "Static";
      break;
    case TUF_MODE_BREATHING:
      modeStr = "Breathing";
      break;
    case TUF_MODE_COLOR_CYCLE:
      modeStr = "Rainbow";
      break;
    case TUF_MODE_STROBE:
      modeStr = "Strobing";
      break;
    default:
      modeStr = "Static";
      break;
    }

    // SMART POLLING:
    // If hardware reports "Static" (0), but our App is in a different mode
    // (e.g. Rainbow), IGNORE the hardware reading. This fixes the issue where
    // hardware incorrectly reports Static. We ONLY sync if hardware reports a
    // NON-STATIC mode, OR if we are already in Static mode.
    QString currentAppMode = m_settings->value("mode").toString();
    if (modeStr == "Static" && currentAppMode != "Static") {
      // Ignore this reading, it's likely false positive from hardware
    } else {
      // Check for Mode Change
      if (m_lastReadMode != modeStr) {
        m_lastReadMode = modeStr;
        // Update settings and emit signal
        if (m_settings->value("mode").toString() != modeStr) {
          m_settings->setValue("mode", modeStr);
          emit modeChanged(modeStr);
        }
      }
    }

    // Check for Brightness Change
    if (m_lastReadBrightness != state.brightness) {
      m_lastReadBrightness = state.brightness;
      // Update settings and emit signal
      if (m_settings->value("brightness").toInt() != state.brightness) {
        m_settings->setValue("brightness", state.brightness);
        emit brightnessChanged(state.brightness);
      }
    }

    // IGNORE Hardware Color (state.color) because it is hardcoded to Red.
    // We keep the existing saved color in m_settings.
  }
}

AuraController::~AuraController() { stopStrobing(); }

void AuraController::stopStrobing() {
  if (m_strobeTimer->isActive()) {
    m_strobeTimer->stop();
  }
}

void AuraController::onStrobeTimeout() {
  m_strobeToggle = !m_strobeToggle;
  QColor c = parseColor(m_strobeColor);
  if (!c.isValid())
    c = QColor(255, 0, 0);

  TufColor tufColor;
  if (m_strobeToggle) {
    tufColor = {static_cast<unsigned char>(c.red()),
                static_cast<unsigned char>(c.green()),
                static_cast<unsigned char>(c.blue())};
  } else {
    tufColor = {0, 0, 0};
  }
  AtkAcpi::instance().setTufKeyboardRGB(TUF_MODE_STATIC, tufColor, 0);
}

void AuraController::applyMode(QString mode, QString color, int speed) {
  if (mode == "Static")
    setStatic(color);
  else if (mode == "Breathing")
    setBreathing(color, speed);
  else if (mode == "Rainbow")
    setRainbow(speed);
  else if (mode == "Strobing")
    setPulsing(color, speed);
  else
    setStatic(color);
}

void AuraController::setBrightness(int level) {
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;
  AtkAcpi::instance().setTufKeyboardBrightness(level);
}

void AuraController::setStatic(QString color) {
  stopStrobing();
  QColor c = parseColor(color);
  if (!c.isValid())
    return;

  TufColor tufColor = {static_cast<unsigned char>(c.red()),
                       static_cast<unsigned char>(c.green()),
                       static_cast<unsigned char>(c.blue())};
  AtkAcpi::instance().setTufKeyboardRGB(TUF_MODE_STATIC, tufColor, 0);
}

void AuraController::setBreathing(QString color, int speed) {
  stopStrobing();
  QColor c = parseColor(color);
  if (!c.isValid())
    return;

  TufColor tufColor = {static_cast<unsigned char>(c.red()),
                       static_cast<unsigned char>(c.green()),
                       static_cast<unsigned char>(c.blue())};

  int tufSpeed = TUF_SPEED_MEDIUM;
  if (speed <= 0)
    tufSpeed = TUF_SPEED_SLOW;
  else if (speed >= 2)
    tufSpeed = TUF_SPEED_FAST;

  AtkAcpi::instance().setTufKeyboardRGB(TUF_MODE_BREATHING, tufColor, tufSpeed);
}

void AuraController::setRainbow(int speed) {
  stopStrobing();
  TufColor tufColor = {0xFF, 0xFF, 0xFF};

  int tufSpeed = TUF_SPEED_MEDIUM;
  if (speed <= 0)
    tufSpeed = TUF_SPEED_SLOW;
  else if (speed >= 2)
    tufSpeed = TUF_SPEED_FAST;

  AtkAcpi::instance().setTufKeyboardRGB(TUF_MODE_COLOR_CYCLE, tufColor,
                                        tufSpeed);
}

void AuraController::setPulsing(QString color, int speed) {
  stopStrobing();
  m_strobeColor = color;
  m_strobeToggle = true;

  int interval;
  if (speed <= 1)
    interval = 1000;
  else if (speed == 2)
    interval = 300;
  else
    interval = 100;

  QColor c = parseColor(color);
  if (c.isValid()) {
    TufColor tufColor = {static_cast<unsigned char>(c.red()),
                         static_cast<unsigned char>(c.green()),
                         static_cast<unsigned char>(c.blue())};
    AtkAcpi::instance().setTufKeyboardRGB(TUF_MODE_STATIC, tufColor, 0);
  }
  m_strobeTimer->start(interval);
}

void AuraController::restoreServices(QString mode, QString color) {
  applyMode(mode, color, getLastSpeed());
}

QColor AuraController::parseColor(QString colorHex) {
  if (colorHex.startsWith("#"))
    colorHex.remove(0, 1);
  return QColor("#" + colorHex);
}

void AuraController::saveState(QString mode, QString color, int brightness,
                               int speed) {
  m_settings->setValue("mode", mode);
  m_settings->setValue("color", color);
  m_settings->setValue("brightness", brightness);
  m_settings->setValue("speed", speed);
}

int AuraController::getSystemBrightness() { return getHardwareBrightness(); }
QString AuraController::getLastMode() {
  return m_settings->value("mode", "Static").toString();
}
QString AuraController::getLastColor() {
  return m_settings->value("color", "FF0000").toString();
}
int AuraController::getLastBrightness() {
  return m_settings->value("brightness", 3).toInt();
}
int AuraController::getLastSpeed() {
  return m_settings->value("speed", 1).toInt();
}
int AuraController::getHardwareBrightness() {
  return AtkAcpi::instance().getTufKeyboardBrightness();
}
bool AuraController::isConnected() { return AtkAcpi::instance().isConnected(); }
