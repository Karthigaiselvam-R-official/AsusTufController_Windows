#include "SystemStatsMonitor.h"
#include "platform/windows/WmiWrapper.h"

#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <shlobj.h> // For SHGetKnownFolderPath if needed
#include <QDebug>
#include <QSysInfo>
#include <QSettings>
#include <QStorageInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDir>
#include <cmath>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "user32.lib")

SystemStatsMonitor::SystemStatsMonitor(QObject *parent) : QObject(parent)
{
    // Initialize PDH for CPU Stats
    initPdh();
    fetchStaticInfo();

    // 1. Main Stats Timer (500ms) - Fast polled items
    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this, &SystemStatsMonitor::updateStats);
    m_timer->start();
    
    // 2. Slow Timer (2000ms) - Heavy I/O (Disk, Net)
    m_slowTimer = new QTimer(this);
    m_slowTimer->setInterval(2000);
    connect(m_slowTimer, &QTimer::timeout, this, &SystemStatsMonitor::updateSlowStats);
    m_slowTimer->start();
    
    // 3. Enforcement Timer (5000ms) - Ensure charge limit sticks (common issue on Windows too)
    m_enforcementTimer = new QTimer(this);
    m_enforcementTimer->setInterval(5000);
    connect(m_enforcementTimer, &QTimer::timeout, this, &SystemStatsMonitor::enforceChargeLimit);
    m_enforcementTimer->start();

    // 4. GPU Process Init - Using nvidia-smi (Cross-platform tool)
    // If not present, we will fallback to 0 or try other methods later
    m_gpuProcess = new QProcess(this);
    connect(m_gpuProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SystemStatsMonitor::onGpuProcessFinished);
    
    // 5. Debounce Timer for Sliders
    m_limitDebounceTimer = new QTimer(this);
    m_limitDebounceTimer->setSingleShot(true);
    m_limitDebounceTimer->setInterval(500);
    connect(m_limitDebounceTimer, &QTimer::timeout, this, &SystemStatsMonitor::applyPendingChargeLimit);

    // 6. MTP Worker (Stubbed for Windows - Logic handled by AutoPlay/WPD usually)
    m_mtpThread = new QThread(this);
    m_mtpWorker = new MtpWorker();
    m_mtpWorker->moveToThread(m_mtpThread);
    connect(m_mtpThread, &QThread::started, m_mtpWorker, &MtpWorker::start);
    connect(m_mtpWorker, &MtpWorker::devicesFound, this, &SystemStatsMonitor::onMtpDevicesFound);
    connect(m_mtpThread, &QThread::finished, m_mtpWorker, &QObject::deleteLater);
    m_mtpThread->start();
    
    // Initial Reads
    readChargeLimit();
    
    // Restore Saved Limit
    QSettings settings("AsusTuf", "FanControl");
    int savedLimit = settings.value("ChargeLimit", -1).toInt();
    if (savedLimit >= 40 && savedLimit <= 100) {
        if (m_chargeLimit != savedLimit) {
            qDebug() << "Restoring saved charge limit:" << savedLimit;
            setChargeLimit(savedLimit);
        }
    }
    
    // Initial Update
    updateStats();
    updateSlowStats();
}

SystemStatsMonitor::~SystemStatsMonitor()
{
    if (m_mtpThread) {
        m_mtpThread->quit();
        m_mtpThread->wait(1000);
    }
    
    if (m_gpuProcess && m_gpuProcess->state() != QProcess::NotRunning) {
        m_gpuProcess->kill();
    }
    
    if (m_pdhQuery) {
        PdhCloseQuery((PDH_HQUERY)m_pdhQuery);
    }
}

void SystemStatsMonitor::initPdh()
{
    // Open PDH Query for CPU Usage
    if (PdhOpenQuery(NULL, NULL, (PDH_HQUERY*)&m_pdhQuery) == ERROR_SUCCESS) {
        // Add "processor time" counter
        // Note: Using English counter path to work on non-English Windows
        PdhAddEnglishCounter((PDH_HQUERY)m_pdhQuery, L"\\Processor(_Total)\\% Processor Time", NULL, (PDH_HCOUNTER*)&m_pdhCpuCounter);
        // Initial collect
        PdhCollectQueryData((PDH_HQUERY)m_pdhQuery);
    }
}

void SystemStatsMonitor::fetchStaticInfo()
{
    // 1. Laptop Model 
    // Fallback if WMI fails
    m_laptopModel = "ASUS System";
    
    // 2. OS Version
    m_osVersion = QSysInfo::prettyProductName();
    
    // 3. CPU Model (Registry)
    QSettings cpuKey("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", QSettings::NativeFormat);
    m_cpuModel = cpuKey.value("ProcessorNameString", "Unknown CPU").toString();
    
    // 4. GPU Models (EnumDisplayDevices)
    m_gpuModels.clear();
    DISPLAY_DEVICEW dd;
    dd.cb = sizeof(dd);
    int deviceIndex = 0;
    while (EnumDisplayDevicesW(NULL, deviceIndex, &dd, 0)) {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE || dd.StateFlags & DISPLAY_DEVICE_ACTIVE) {
             QString gpuName = QString::fromWCharArray(dd.DeviceString);
             if (!m_gpuModels.contains(gpuName) && !gpuName.isEmpty())
                 m_gpuModels.append(gpuName);
        }
        deviceIndex++;
    }
    if (m_gpuModels.isEmpty()) m_gpuModels.append("Generic Graphics Adapter");
}

void SystemStatsMonitor::updateStats()
{
    readCpuUsage();
    readMemoryUsage();
    readGpuStats(); // Async
    readBattery();
    
    // CPU Freq approximation (Windows doesn't easily give real-time per core freq without WMI/Performance Ctr overhead)
    // We'll trust the OS reported max or base for now, or improve later
    
    emit statsChanged();
}

void SystemStatsMonitor::updateSlowStats()
{
    readDiskUsage();
    readNetworkUsage();
    emit statsChanged();
}

void SystemStatsMonitor::readCpuUsage()
{
    if (m_pdhQuery && m_pdhCpuCounter) {
        PdhCollectQueryData((PDH_HQUERY)m_pdhQuery);
        PDH_FMT_COUNTERVALUE counterVal;
        if (PdhGetFormattedCounterValue((PDH_HCOUNTER)m_pdhCpuCounter, PDH_FMT_LONG, NULL, &counterVal) == ERROR_SUCCESS) {
            m_cpuUsage = (int)counterVal.longValue;
        }
    }
}

void SystemStatsMonitor::readMemoryUsage()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_ramUsage = memInfo.dwMemoryLoad;
    }
}

void SystemStatsMonitor::readGpuStats()
{
    // Using nvidia-smi if available
    if (m_gpuProcess->state() != QProcess::NotRunning) return;
    
    // Standard params: Clock, Utilization
    m_gpuProcess->start("nvidia-smi", QStringList() << "--query-gpu=clocks.gr,utilization.gpu" << "--format=csv,noheader,nounits");
}

void SystemStatsMonitor::onGpuProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (exitCode != 0) return;
    
    QString output = m_gpuProcess->readAllStandardOutput().trimmed();
    if (output.isEmpty()) return;
    
    // Output format: "1200, 45" (Freq, Usage)
    QStringList parts = output.split(",");
    if (parts.length() >= 2) {
        m_gpuFreq = parts[0].trimmed().toDouble();
        m_gpuUsage = parts[1].trimmed().toInt();
    }
    else if (parts.length() == 1) {
        // Sometimes only one value comes back depending on query
         m_gpuUsage = parts[0].trimmed().toInt();
    }
}

void SystemStatsMonitor::readBattery()
{
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        m_batteryPercent = sps.BatteryLifePercent;
        if (m_batteryPercent > 100) m_batteryPercent = 100; // 255 usually means unknown
        if (m_batteryPercent == 255) m_batteryPercent = 0; 
        
        m_isCharging = (sps.ACLineStatus == 1);
        
        if (m_isCharging) {
            if (m_batteryPercent >= m_chargeLimit - 2)
                m_batteryState = "Full (Limit)"; // Stopped charging
            else
                m_batteryState = "Charging";
        } else {
            m_batteryState = "Discharging";
        }
    }
}

void SystemStatsMonitor::readNetworkUsage()
{
    MIB_IFTABLE *pIfTable;
    DWORD dwSize = 0;
    
    pIfTable = (MIB_IFTABLE*)malloc(sizeof(MIB_IFTABLE));
    if (!pIfTable) return;
    
    // Get size
    if (GetIfTable(pIfTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE*)malloc(dwSize);
        if (!pIfTable) return;
    }

    if (GetIfTable(pIfTable, &dwSize, 0) == NO_ERROR) {
        quint64 totalIn = 0;
        quint64 totalOut = 0;
        
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            MIB_IFROW row = pIfTable->table[i];
            // Filter inactive interfaces
            if (row.dwType != MIB_IF_TYPE_LOOPBACK && 
                row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL &&
                (row.dwInOctets > 0 || row.dwOutOctets > 0)) {
                
                totalIn += row.dwInOctets;
                totalOut += row.dwOutOctets;
            }
        }
        
        if (m_lastNetBytesIn > 0) {
            // Speed = Delta / Time (2.0s)
            // Result in Bytes/sec
            double inBytesPerSec = (totalIn - m_lastNetBytesIn) / 2.0;
            double outBytesPerSec = (totalOut - m_lastNetBytesOut) / 2.0;
            
            // Convert to KB/s for UI
            m_netDown = inBytesPerSec / 1024.0;
            m_netUp = outBytesPerSec / 1024.0;
        }
        
        m_lastNetBytesIn = totalIn;
        m_lastNetBytesOut = totalOut;
    }
    free(pIfTable);
}

void SystemStatsMonitor::readDiskUsage()
{
    QVariantList drives;
    double totalAll = 0;
    double usedAll = 0;

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            QVariantMap drive;
            
            QString root = storage.rootPath();
            QString name = storage.displayName();
            if (name.isEmpty()) name = root; // e.g. "C:/"
            
            double totalGB = storage.bytesTotal() / (1024.0 * 1024.0 * 1024.0);
            double freeGB = storage.bytesAvailable() / (1024.0 * 1024.0 * 1024.0);
            double usedGB = totalGB - freeGB;
            double usagePct = (totalGB > 0) ? (usedGB / totalGB) * 100.0 : 0;
            
            drive["name"] = name;
            drive["mount"] = root; // Used for opening
            drive["device"] = storage.device(); 
            
            drive["total"] = QString::number(totalGB, 'f', 1);
            drive["used"] = QString::number(usedGB, 'f', 1);
            drive["free"] = QString::number(freeGB, 'f', 1);
            drive["usage"] = usagePct;
            drive["freePercent"] = 100.0 - usagePct;
            drive["hasUsage"] = true;
            drive["isMounted"] = true;
            
            drives.append(drive);
            
            totalAll += totalGB;
            usedAll += usedGB;
        }
    }
    m_diskPartitions = drives;
    
    // Update Global Disk Usage
    if (totalAll > 0) {
        m_diskUsage = (int)((usedAll / totalAll) * 100.0);
    } else {
        m_diskUsage = 0;
    }
}

// --- Charge Limit Logic ---

void SystemStatsMonitor::readChargeLimit()
{
    // On Windows, reading the CURRENT limit is often done via WMI
    // ID: 0x00120057 (Battery Charge Limit)
    // We'll interpret -1 as fail
    
    // For now, we trust our internal state or QSettings as source of truth
    // because reading back from WMI "DEVS" return value is complex (often returns status bits).
    // But we CAN try to reading it if we implement the WMI Get method properly.
    
    // Placeholder: Assume 80 if unknown
}

void SystemStatsMonitor::setChargeLimit(int limit)
{
    if (limit < 40) limit = 40; // some ASUS laptops limit min to 40 or 60
    if (limit > 100) limit = 100;
    
    m_pendingChargeLimit = limit;
    m_chargeLimit = limit; // Optimistic update
    
    // Debounce actual WMI call
    m_limitDebounceTimer->start();
    
    emit chargeLimitChanged();
}

void SystemStatsMonitor::applyPendingChargeLimit()
{
    writeChargeLimitToWmi(m_pendingChargeLimit);
    
    // Save to Settings
    QSettings settings("AsusTuf", "FanControl");
    settings.setValue("ChargeLimit", m_pendingChargeLimit);
}

void SystemStatsMonitor::enforceChargeLimit()
{
    // Periodically re-apply just in case Windows or another app reset it
    writeChargeLimitToWmi(m_chargeLimit);
}

void SystemStatsMonitor::writeChargeLimitToWmi(int limit)
{
    // Device ID for Battery Charge Limit: 0x00120057
    // This is standard for modern ASUS laptops (G-Helper uses this)
    WmiWrapper::instance().setDevice(0x00120057, limit);
}

void SystemStatsMonitor::openFileManager(const QString &path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void SystemStatsMonitor::openFileManager(const QString &mount, const QString &device)
{
    Q_UNUSED(device);
    QDesktopServices::openUrl(QUrl::fromLocalFile(mount));
}

void SystemStatsMonitor::onMtpDevicesFound(QVariantList devices)
{
    m_cachedMtpDevices = devices;
    // trigger stats update to merge list? 
    // For now we just store it.
}
