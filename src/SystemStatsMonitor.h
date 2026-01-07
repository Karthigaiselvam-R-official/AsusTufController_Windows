#ifndef SYSTEMSTATSMONITOR_H
#define SYSTEMSTATSMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QProcess>
#include <QThread>
#include <QVariantList>
#include <QStorageInfo>

// Forward decl or nested MtpWorker if we want to keep it simple
class MtpWorker : public QObject {
    Q_OBJECT
public:
    explicit MtpWorker(QObject *parent = nullptr) : QObject(parent) {}
public slots:
    void start() {
        // Mock implementation for Windows for now
        // On Windows, MTP devices often show up as Drives or Portable Devices via WPD
        // We will just emit empty or simulate
        emit devicesFound({}); 
    }
signals:
    void devicesFound(QVariantList devices);
};

class SystemStatsMonitor : public QObject
{
    Q_OBJECT
    
    // Core Properties
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryStatsChanged)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY batteryStatsChanged)
    Q_PROPERTY(QString batteryState READ batteryState NOTIFY batteryStatsChanged)
    Q_PROPERTY(int chargeLimit READ chargeLimit WRITE setChargeLimit NOTIFY chargeLimitChanged)
    
    // Live Stats
    Q_PROPERTY(int cpuUsage READ cpuUsage NOTIFY statsChanged)
    Q_PROPERTY(int ramUsage READ ramUsage NOTIFY statsChanged)
    Q_PROPERTY(int diskUsage READ diskUsage NOTIFY statsChanged)
    Q_PROPERTY(int gpuUsage READ gpuUsage NOTIFY statsChanged)
    Q_PROPERTY(double cpuFreq READ cpuFreq NOTIFY statsChanged)
    Q_PROPERTY(double gpuFreq READ gpuFreq NOTIFY statsChanged)
    
    // Static Info
    Q_PROPERTY(QString laptopModel READ laptopModel CONSTANT)
    Q_PROPERTY(QString osVersion READ osVersion CONSTANT)
    Q_PROPERTY(QString cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(QStringList gpuModels READ gpuModels CONSTANT)
    
    // Complex Data
    Q_PROPERTY(QVariantList diskPartitions READ diskPartitions NOTIFY statsChanged)
    Q_PROPERTY(double netDown READ netDown NOTIFY statsChanged) // KB/s
    Q_PROPERTY(double netUp READ netUp NOTIFY statsChanged)     // KB/s

public:
    explicit SystemStatsMonitor(QObject *parent = nullptr);
    ~SystemStatsMonitor();

    // Getters
    int batteryPercent() const { return m_batteryPercent; }
    bool isCharging() const { return m_isCharging; }
    QString batteryState() const { return m_batteryState; }
    int chargeLimit() const { return m_chargeLimit; }
    
    int cpuUsage() const { return m_cpuUsage; }
    int ramUsage() const { return m_ramUsage; }
    int diskUsage() const { return m_diskUsage; }
    int gpuUsage() const { return m_gpuUsage; }
    double cpuFreq() const { return m_cpuFreq; }
    double gpuFreq() const { return m_gpuFreq; }
    
    QString laptopModel() const { return m_laptopModel; }
    QString osVersion() const { return m_osVersion; }
    QString cpuModel() const { return m_cpuModel; }
    QStringList gpuModels() const { return m_gpuModels; }
    
    QVariantList diskPartitions() const { return m_diskPartitions; }
    double netDown() const { return m_netDown; }
    double netUp() const { return m_netUp; }

    Q_INVOKABLE void openFileManager(const QString &path);
    Q_INVOKABLE void openFileManager(const QString &mount, const QString &device); // Overload for drive click

    void setChargeLimit(int limit);

signals:
    void batteryStatsChanged();
    void chargeLimitChanged();
    void statsChanged();

private slots:
    void updateStats();         // Fast (500ms)
    void updateSlowStats();     // Slow (2000ms)
    void enforceChargeLimit();  // Check (5000ms)
    void applyPendingChargeLimit(); // Debounced
    
    void onGpuProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onMtpDevicesFound(QVariantList devices);

private:
    // --- Member Variables ---
    
    // Data
    int m_batteryPercent = 0;
    bool m_isCharging = false;
    QString m_batteryState = "Unknown";
    int m_chargeLimit = 80;
    int m_pendingChargeLimit = 80;
    
    int m_cpuUsage = 0;
    int m_ramUsage = 0;
    int m_diskUsage = 0; // Overall %
    int m_gpuUsage = 0;
    double m_cpuFreq = 0;
    double m_gpuFreq = 0;
    
    QString m_laptopModel;
    QString m_osVersion;
    QString m_cpuModel;
    QStringList m_gpuModels;
    
    QVariantList m_diskPartitions;
    QVariantList m_cachedMtpDevices;
    
    double m_netDown = 0;
    double m_netUp = 0;
    
    // State Tracking
    int m_cachedVolumeCount = 0;
    quint64 m_lastNetBytesIn = 0;
    quint64 m_lastNetBytesOut = 0;
    double m_diskTotal = 0;
    double m_diskUsed = 0;

    // Timers
    QTimer *m_timer = nullptr;
    QTimer *m_slowTimer = nullptr;
    QTimer *m_enforcementTimer = nullptr;
    QTimer *m_limitDebounceTimer = nullptr;
    
    // Async Process
    QProcess *m_gpuProcess = nullptr;
    
    // Threading
    QThread *m_mtpThread = nullptr;
    MtpWorker *m_mtpWorker = nullptr;
    
    // Implementation Helpers (Windows Specific)
    void initPdh();
    void fetchStaticInfo();
    
    // Read Methods
    void readCpuUsage();
    void readCpuFreq();
    void readMemoryUsage();
    void readGpuStats();
    void readBattery();
    void readDiskUsage();
    void readNetworkUsage();
    void readChargeLimit(); // WMI read
    
    // Write Methods
    void writeChargeLimitToWmi(int limit);
    
    // Windows PDH Handles
    void* m_pdhQuery = nullptr;
    void* m_pdhCpuCounter = nullptr;
};

#endif // SYSTEMSTATSMONITOR_H
