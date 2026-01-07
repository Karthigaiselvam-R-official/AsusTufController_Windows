#ifndef AURACONTROLLER_H
#define AURACONTROLLER_H

#include <QObject>
#include <QString>
#include <QSettings>

class AuraController : public QObject
{
    Q_OBJECT
public:
    explicit AuraController(QObject *parent = nullptr);

    // Invokables called by AuraPage.qml
    Q_INVOKABLE void setBrightness(int level); // 0-3
    Q_INVOKABLE void setStatic(QString color); // Hex string "FF0000"
    Q_INVOKABLE void setBreathing(QString color, int speed);
    Q_INVOKABLE void setRainbow(int speed);
    Q_INVOKABLE void setPulsing(QString color, int speed); // Strobing
    
    Q_INVOKABLE void saveState(QString mode, QString color);
    Q_INVOKABLE int getSystemBrightness();
    Q_INVOKABLE QString getLastMode();
    Q_INVOKABLE QString getLastColor();

private:
    void setMode(int mode); // 0: Static, 1: Breathing, 2: Rainbow, 3: Strobe
    void setColor(QString colorHex);
    void setSpeed(int speed);

    QSettings *m_settings;
};

#endif // AURACONTROLLER_H
