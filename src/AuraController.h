#ifndef AURACONTROLLER_H
#define AURACONTROLLER_H

#include <QColor>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>

class AuraController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool connected READ isConnected CONSTANT)

public:
  explicit AuraController(QObject *parent = nullptr);
  ~AuraController();

  Q_INVOKABLE void initFromHardware();

  Q_INVOKABLE void setBrightness(int level);
  Q_INVOKABLE void setStatic(QString color);
  Q_INVOKABLE void setBreathing(QString color, int speed);
  Q_INVOKABLE void setRainbow(int speed);
  Q_INVOKABLE void setPulsing(QString color, int speed);

  Q_INVOKABLE void saveState(QString mode, QString color, int brightness,
                             int speed);
  Q_INVOKABLE QString getLastMode();
  Q_INVOKABLE QString getLastColor();
  Q_INVOKABLE int getLastBrightness();
  Q_INVOKABLE int getLastSpeed();
  Q_INVOKABLE int getHardwareBrightness();
  Q_INVOKABLE void restoreServices(QString mode, QString color);
  Q_INVOKABLE int getSystemBrightness();
  Q_INVOKABLE bool isConnected();

signals:
  void modeChanged(QString mode);
  void brightnessChanged(int level);

private slots:
  void onStrobeTimeout();
  void onPollTimeout();

private:
  QColor parseColor(QString colorHex);
  void applyMode(QString mode, QString color, int speed);
  void stopStrobing();

  QSettings *m_settings;
  QTimer *m_strobeTimer;
  QTimer *m_pollTimer;
  QString m_strobeColor;
  bool m_strobeToggle;

  // Previous confirmed state (to avoid signal spam)
  QString m_lastReadMode;
  int m_lastReadBrightness;
  bool m_initializedFromSettings;
};

#endif
