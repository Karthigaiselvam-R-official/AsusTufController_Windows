#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>

class FanController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpuFanRpm READ cpuFanRpm NOTIFY cpuFanRpmChanged)
    Q_PROPERTY(int gpuFanRpm READ gpuFanRpm NOTIFY gpuFanRpmChanged)
    Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
    Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
    Q_PROPERTY(bool isManualModeActive READ isManualModeActive NOTIFY isManualModeActiveChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit FanController(QObject *parent = nullptr);
    
    // Properties interaction
    int cpuFanRpm() const { return m_cpuFanRpm; }
    int gpuFanRpm() const { return m_gpuFanRpm; }
    int cpuTemp() const { return m_cpuTemp; }
    int gpuTemp() const { return m_gpuTemp; }
    bool isManualModeActive() const { return m_isManualModeActive; }
    QString statusMessage() const { return m_statusMessage; }

    // Invokables for QML
    Q_INVOKABLE void setFanSpeed(int percentage);
    Q_INVOKABLE void enableAutoMode();

signals:
    void cpuFanRpmChanged();
    void gpuFanRpmChanged();
    void cpuTempChanged();
    void gpuTempChanged();
    void isManualModeActiveChanged();
    void statusMessageChanged();

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
    
    QTimer *m_updateTimer;
};

#endif // FANCONTROLLER_H
