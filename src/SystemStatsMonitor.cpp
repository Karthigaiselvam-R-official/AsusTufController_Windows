#include "SystemStatsMonitor.h"
#include "platform/windows/AsusWinIO.h"
#include "platform/windows/WmiWrapper.h"

// Qt headers
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QUrl>
#include <cmath>

// Windows headers - ORDER MATTERS! windows.h must come first
// Define Windows version for GetIfTable2 support (Vista+)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <iphlpapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <powrprof.h> // For CallNtPowerInformation
#include <psapi.h>
#include <shlobj.h>
#include <vector>
#include <windows.h>

// WPD (Portable Devices)
#include <PortableDevice.h>
#include <PortableDeviceApi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "powrprof.lib")            // Required for Power Info
#pragma comment(lib, "PortableDeviceGuids.lib") // Required for WPD CLSIDs
#pragma comment(lib, "user32.lib")

// Struct for Power Info if not fully defined in MinGW headers
typedef struct _PROCESSOR_POWER_INFORMATION_EX {
  ULONG Number;
  ULONG MaxMhz;
  ULONG CurrentMhz;
  ULONG MhzLimit;
  ULONG MaxIdleState;
  ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION_EX, *PPROCESSOR_POWER_INFORMATION_EX;

SystemStatsMonitor::SystemStatsMonitor(QObject *parent) : QObject(parent) {
  // Initialize PDH for CPU Stats
  initPdh();
  fetchStaticInfo();

  // 1. Main Stats Timer (500ms) - Match Linux Implementation for snappy updates
  m_timer = new QTimer(this);
  m_timer->setInterval(500);
  connect(m_timer, &QTimer::timeout, this, &SystemStatsMonitor::updateStats);
  m_timer->start();

  // 2. Slow Timer (2000ms) - Heavy I/O (Disk, Net)
  m_slowTimer = new QTimer(this);
  m_slowTimer->setInterval(2000);
  connect(m_slowTimer, &QTimer::timeout, this,
          &SystemStatsMonitor::updateSlowStats);
  m_slowTimer->start();

  // 3. Enforcement Timer (200ms) - Ensure charge limit sticks (common issue on
  // Windows too)
  m_enforcementTimer = new QTimer(this);
  m_enforcementTimer->setInterval(200);
  connect(m_enforcementTimer, &QTimer::timeout, this,
          &SystemStatsMonitor::enforceChargeLimit);
  m_enforcementTimer->start();

  // 4. GPU Process Init - Using nvidia-smi (Cross-platform tool)
  // If not present, we will fallback to 0 or try other methods later
  m_gpuProcess = new QProcess(this);
  connect(m_gpuProcess,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &SystemStatsMonitor::onGpuProcessFinished);

  // 5. Debounce Timer for Sliders
  m_limitDebounceTimer = new QTimer(this);
  m_limitDebounceTimer->setSingleShot(true);
  m_limitDebounceTimer->setInterval(200);
  connect(m_limitDebounceTimer, &QTimer::timeout, this,
          &SystemStatsMonitor::applyPendingChargeLimit);

  // 6. MTP Worker (Stubbed for Windows - Logic handled by AutoPlay/WPD usually)
  m_mtpThread = new QThread(this);
  m_mtpWorker = new MtpWorker();
  m_mtpWorker->moveToThread(m_mtpThread);
  connect(m_mtpThread, &QThread::started, m_mtpWorker, &MtpWorker::start);
  connect(m_mtpWorker, &MtpWorker::devicesFound, this,
          &SystemStatsMonitor::onMtpDevicesFound);
  connect(m_mtpThread, &QThread::finished, m_mtpWorker, &QObject::deleteLater);
  m_mtpThread->start();

  // Initial Reads
  readChargeLimit();

  // Restore Saved Limit
  QSettings settings("AsusTuf", "FanControl");
  int savedLimit = settings.value("ChargeLimit", -1).toInt();
  if (savedLimit >= 60 && savedLimit <= 100) {
    if (m_chargeLimit != savedLimit) {
      setChargeLimit(savedLimit);
    }
  }

  // Initial Update
  updateStats();
  updateSlowStats();
}

SystemStatsMonitor::~SystemStatsMonitor() {
  if (m_mtpThread) {
    m_mtpThread->quit();
    m_mtpThread->wait(1000);
  }

  if (m_gpuProcess && m_gpuProcess->state() != QProcess::NotRunning) {
    m_gpuProcess->terminate(); // Try graceful first
    if (!m_gpuProcess->waitForFinished(200)) {
      m_gpuProcess->kill(); // Force kill if stuck
    }
  }

  if (m_pdhQuery) {
    PdhCloseQuery((PDH_HQUERY)m_pdhQuery);
  }
}

void SystemStatsMonitor::initPdh() {
  // Initialize PDH Query
  if (PdhOpenQuery(NULL, 0, (PDH_HQUERY *)&m_pdhQuery) != ERROR_SUCCESS) {
    m_pdhQuery = nullptr;
    return;
  }

  // Add Universal GPU Utilization Counter (Wildcard for all engines)
  // We use PdhAddEnglishCounterW (Vista+) to ensure it works on non-English
  // systems Path: \GPU Engine(*)\Utilization Percentage
  if (PdhAddEnglishCounterW(
          (PDH_HQUERY)m_pdhQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0,
          (PDH_HCOUNTER *)&m_pdhGpuCounter) != ERROR_SUCCESS) {
    // Retry with Localized name if English fails (Fallback)
    // But usually English API works. If it fails, GPU counters might be
    // missing.
    m_pdhGpuCounter = nullptr;
  }

  // Add Processor Frequency Counter
  // \Processor Information(_Total)\Processor Frequency
  // This returns the current frequency in MHz
  // We use "% of Maximum Frequency" because "Processor Frequency" can be
  // misleading on some Windows versions
  if (PdhAddEnglishCounterW(
          (PDH_HQUERY)m_pdhQuery,
          L"\\Processor Information(_Total)\\% of Maximum Frequency", 0,
          (PDH_HCOUNTER *)&m_pdhCpuFreqCounter) != ERROR_SUCCESS) {
    m_pdhCpuFreqCounter = nullptr;
  }
}

void SystemStatsMonitor::fetchStaticInfo() {
  // 1. Laptop Model
  // Fallback if WMI fails
  // On Linux: Reads /sys/class/dmi/id/product_name
  // On Windows: We can try Registry or keep "ASUS System" if WMI fails later
  m_laptopModel = "ASUS System";

  // Try to read from Registry for more accurate model if possible
  QSettings systemKey("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS",
                      QSettings::NativeFormat);
  QString model = systemKey.value("SystemProductName").toString();
  if (!model.isEmpty()) {
    m_laptopModel = model;
  }

  // 2. OS Version
  // On Linux: /etc/os-release
  // On Windows: QSysInfo is good, but let's be verbose to match Linux "Parsing"
  // style
  m_osVersion = QSysInfo::prettyProductName();
  QString build = QSysInfo::kernelVersion();
  if (!build.isEmpty()) {
    m_osVersion += " (Build " + build + ")";
  }

  // 3. CPU Model
  // On Linux: /proc/cpuinfo parsing
  // On Windows: Registry
  QSettings cpuKey(
      "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
      QSettings::NativeFormat);
  m_cpuModel = cpuKey.value("ProcessorNameString", "Unknown CPU").toString();
  // Cleanup CPU string (remove extra spaces like Linux does)
  m_cpuModel = m_cpuModel.simplified();

  // 4. Max CPU Frequency (Base)
  int numCpus = QThread::idealThreadCount();
  if (numCpus <= 0)
    numCpus = 1;
  int size = numCpus * sizeof(PROCESSOR_POWER_INFORMATION_EX);
  std::vector<PROCESSOR_POWER_INFORMATION_EX> buffer(numCpus);
  // SystemProcessorPowerInformation = 11
  long status = CallNtPowerInformation(static_cast<POWER_INFORMATION_LEVEL>(11),
                                       NULL, 0, buffer.data(), size);

  if (status == 0 && buffer.size() > 0) {
    // MaxMhz is usually the same for all cores on consumer CPUs
    m_maxCpuMhz = buffer[0].MaxMhz;
  }
  if (m_maxCpuMhz <= 0) {
    m_maxCpuMhz = 2500; // Fallback
  }

  // 4. GPU Models
  // On Linux: lspci parsing
  // On Windows: EnumDisplayDevicesW
  m_gpuModels.clear();
  DISPLAY_DEVICEW dd;
  dd.cb = sizeof(dd);
  int deviceIndex = 0;

  // Iterate all display devices to find distinct GPUs
  QSet<QString> uniqueGpus;

  while (EnumDisplayDevicesW(NULL, deviceIndex, &dd, 0)) {
    if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ||
        dd.StateFlags & DISPLAY_DEVICE_ACTIVE) {
      QString gpuName = QString::fromWCharArray(dd.DeviceString);
      if (!gpuName.isEmpty()) {
        // Clean up name (remove "NVIDIA" repetition if present, etc, just like
        // Linux logic)
        gpuName = gpuName.trimmed();
        uniqueGpus.insert(gpuName);
      }
    }
    deviceIndex++;
  }

  // Also try WMI extraction if EnumDisplayDevices missed (e.g. Headless dGPU)
  // (Stubbed here, but logic exists in WmiWrapper)

  if (uniqueGpus.isEmpty()) {
    m_gpuModels.append("Generic Graphics Adapter");
  } else {
    m_gpuModels = uniqueGpus.values();
  }

  // Log the static info
}

void SystemStatsMonitor::updateStats() {
  readCpuUsage();
  readMemoryUsage();
  readGpuStats(); // Async
  readBattery();

  // CPU Freq approximation
  readCpuFreq();
  readCpuTemp();

  emit statsChanged();
}

void SystemStatsMonitor::readCpuTemp() {
  // Read from AsusWinIO Driver
  // Note: Returns 0 or -1 if driver not initialized (requires Admin)
  m_cpuTemp = AsusWinIO::instance().getCpuTemp();
}

// Forward Declaration
QVariantList readWpdDevices();

void SystemStatsMonitor::updateSlowStats() {
  readDiskUsage();
  readNetworkUsage();

  // Append WPD Devices (Real MTP)
  static int wpdCounter = 0;
  static QVariantList cachedWpd;
  if (wpdCounter++ % 10 ==
      0) { // Scan every 5-10s (Disk Usage calls are frequent?)
    // Note: WPD Open can be slow. Ideally run in thread.
    // For now, we run it every 10th update (assuming 2s interval -> 20s)
    cachedWpd = readWpdDevices();
  }
  QVariantList drives =
      m_diskPartitions; // Assuming readDiskUsage populates m_diskPartitions
  for (const QVariant &v : cachedWpd) {
    drives.append(v.toMap());
  }

  m_diskPartitions = drives;

  emit statsChanged();
}

// Helper for WMI that keeps state
void SystemStatsMonitor::readCpuUsage() {
  // Linux Port: Use raw GetSystemTimes (Total - Idle)
  // Logic matches /proc/stat calculation: (totalDiff - idleDiff) / totalDiff
  // NO SMOOTHING - Raw Delta
  static ULARGE_INTEGER lastIdleTime = {0}, lastKernelTime = {0},
                        lastUserTime = {0};
  static bool firstRun = true;

  FILETIME idleTime, kernelTime, userTime;
  if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;

    if (!firstRun) {
      ULONGLONG idleDiff = idle.QuadPart - lastIdleTime.QuadPart;
      ULONGLONG kernelDiff = kernel.QuadPart - lastKernelTime.QuadPart;
      ULONGLONG userDiff = user.QuadPart - lastUserTime.QuadPart;

      // KernelTime INCLUDES IdleTime in Windows
      // Total = Kernel + User
      ULONGLONG totalSystem = kernelDiff + userDiff;

      if (totalSystem > 0) {
        // Active = Total - Idle (same as Linux: totalDiff - idleDiff)
        ULONGLONG active = totalSystem - idleDiff;

        // Calculate raw percentage
        double currentUsage = (static_cast<double>(active) * 100.0) /
                              static_cast<double>(totalSystem);

        // Clamp
        if (currentUsage < 0)
          currentUsage = 0;
        if (currentUsage > 100)
          currentUsage = 100;

        m_cpuUsage = static_cast<int>(std::round(currentUsage));
      }
    }

    lastIdleTime = idle;
    lastKernelTime = kernel;
    lastUserTime = user;
    firstRun = false;
  }
}

void SystemStatsMonitor::readMemoryUsage() {
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  if (GlobalMemoryStatusEx(&memInfo)) {
    // Task Manager shows: (Total Physical - Available Physical) / Total
    // Physical memInfo.ullTotalPhys = total RAM in bytes memInfo.ullAvailPhys =
    // available RAM in bytes
    DWORDLONG usedBytes = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    m_ramUsage = static_cast<int>((usedBytes * 100) / memInfo.ullTotalPhys);
  }
}

void SystemStatsMonitor::readBattery() {
  SYSTEM_POWER_STATUS sps;
  if (GetSystemPowerStatus(&sps)) {
    m_batteryPercent = sps.BatteryLifePercent;
    if (m_batteryPercent > 100)
      m_batteryPercent = 100; // 255 usually means unknown
    if (m_batteryPercent == 255)
      m_batteryPercent = 0;

    // ACLineStatus: 1 = Online, 0 = Offline, 255 = Unknown
    bool isPluggedIn = (sps.ACLineStatus == 1);

    // BatteryFlag: 1=High, 2=Low, 4=Critical, 8=Charging, 128=No System Battery
    // If Plugged In but NOT Charging (Flag 8 not set), it's likely "Full" or
    // "Limit Reached"
    bool isHardwareCharging = (sps.BatteryFlag & 8);

    m_isCharging = isHardwareCharging; // For UI animations

    if (isPluggedIn) {
      if (isHardwareCharging) {
        m_batteryState = "Charging";
      } else {
        // Plugged in but not charging
        if (m_batteryPercent >= m_chargeLimit - 2) {
          m_batteryState = "Full (Limit)";
        } else if (m_batteryPercent >= 95) {
          m_batteryState = "Full";
        } else {
          m_batteryState = "Plugged In";
        }
        m_isCharging = true;
      }
    } else {
      m_batteryState = "Discharging";
      m_isCharging = false;
    }

    // EMIT SIGNAL IF CHANGED so UI updates!
    // We check against old values (implied, or just emit always)
    emit batteryStatsChanged();
  }
}

void SystemStatsMonitor::readNetworkUsage() {
  // Use GetIfTable for network statistics (most compatible API)
  MIB_IFTABLE *pIfTable = nullptr;
  DWORD dwSize = 0;

  // Get required buffer size
  if (GetIfTable(nullptr, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
    pIfTable = (MIB_IFTABLE *)malloc(dwSize);
  }

  if (!pIfTable)
    return;

  if (GetIfTable(pIfTable, &dwSize, FALSE) != NO_ERROR) {
    free(pIfTable);
    return;
  }

  quint64 totalIn = 0;
  quint64 totalOut = 0;

  for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
    MIB_IFROW &row = pIfTable->table[i];

    // Include all operational interfaces except loopback
    // dwType: 6 = Ethernet, 71 = Wi-Fi, 24 = Loopback
    // dwOperStatus: 1 = Up (MIB_IF_OPER_STATUS_OPERATIONAL)
    bool isUp = (row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL);
    bool isLoopback = (row.dwType == MIB_IF_TYPE_LOOPBACK);

    if (isUp && !isLoopback) {
      // dwInOctets and dwOutOctets are 32-bit but sufficient for rate
      // calculation
      totalIn += row.dwInOctets;
      totalOut += row.dwOutOctets;
    }
  }

  free(pIfTable);

  // Calculate speed using member variables
  if (m_lastNetBytesIn > 0) {
    quint64 deltaIn = 0;
    quint64 deltaOut = 0;

    // Handle 32-bit counter wrap
    if (totalIn >= m_lastNetBytesIn) {
      deltaIn = totalIn - m_lastNetBytesIn;
    } else {
      // Counter wrapped, estimate
      deltaIn = (0xFFFFFFFF - m_lastNetBytesIn) + totalIn;
    }

    if (totalOut >= m_lastNetBytesOut) {
      deltaOut = totalOut - m_lastNetBytesOut;
    } else {
      deltaOut = (0xFFFFFFFF - m_lastNetBytesOut) + totalOut;
    }

    // Slow timer runs every 2 seconds
    double inBytesPerSec = static_cast<double>(deltaIn) / 2.0;
    double outBytesPerSec = static_cast<double>(deltaOut) / 2.0;

    // Convert to KB/s (1 KB = 1024 bytes)
    m_netDown = inBytesPerSec / 1024.0;
    m_netUp = outBytesPerSec / 1024.0;
  }

  m_lastNetBytesIn = totalIn;
  m_lastNetBytesOut = totalOut;
}

// Helper to read WPD (Mobile/MTP) Devices
QVariantList readWpdDevices() {
  QVariantList devices;
  HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

  ComPtr<IPortableDeviceManager> deviceManager;
  hr = CoCreateInstance(CLSID_PortableDeviceManager, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&deviceManager));
  if (FAILED(hr))
    return devices;

  DWORD deviceCount = 0;
  hr = deviceManager->GetDevices(NULL, &deviceCount);
  if (FAILED(hr) || deviceCount == 0)
    return devices;

  std::vector<LPWSTR> deviceIds(deviceCount);
  hr = deviceManager->GetDevices(deviceIds.data(), &deviceCount);
  if (FAILED(hr)) {
    for (auto p : deviceIds)
      CoTaskMemFree(p);
    return devices;
  }

  for (DWORD i = 0; i < deviceCount; i++) {
    ComPtr<IPortableDevice> device;
    hr = CoCreateInstance(CLSID_PortableDevice, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&device));
    if (SUCCEEDED(hr)) {
      // Connect to device
      ComPtr<IPortableDeviceValues> clientInfo;
      CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&clientInfo));
      // Minimal Client Info
      if (SUCCEEDED(device->Open(deviceIds[i], clientInfo.Get()))) {
        // Get Friendly Name
        ComPtr<IPortableDeviceContent> content;
        device->Content(&content);

        // Read properties of the DEVICE object (ID: "DEVICE")
        ComPtr<IPortableDeviceProperties> properties;
        content->Properties(&properties);

        ComPtr<IPortableDeviceValues> deviceValues;
        properties->GetValues(L"DEVICE", NULL, &deviceValues);

        LPWSTR friendlyName = NULL;
        deviceValues->GetStringValue(WPD_DEVICE_FRIENDLY_NAME, &friendlyName);
        QString name = (friendlyName) ? QString::fromWCharArray(friendlyName)
                                      : "Unknown Device";
        CoTaskMemFree(friendlyName);

        // Enumerate Storage Objects to get size
        ComPtr<IEnumPortableDeviceObjectIDs> enumObject;
        content->EnumObjects(0, L"DEVICE", NULL, &enumObject);

        DWORD fetched = 0;
        LPWSTR objectIds[10] = {0};
        while (enumObject &&
               SUCCEEDED(enumObject->Next(10, objectIds, &fetched)) &&
               fetched > 0) {
          for (DWORD j = 0; j < fetched; j++) {
            ComPtr<IPortableDeviceValues> storageValues;
            properties->GetValues(objectIds[j], NULL, &storageValues);

            if (storageValues) {
              ULONGLONG capacity = 0;
              ULONGLONG freeSpace = 0;

              // Check if it's storage? (Ideally check WPD_OBJECT_CONTENT_TYPE)
              HRESULT hrCap = storageValues->GetUnsignedLargeIntegerValue(
                  WPD_STORAGE_CAPACITY, &capacity);
              HRESULT hrFree = storageValues->GetUnsignedLargeIntegerValue(
                  WPD_STORAGE_FREE_SPACE_IN_BYTES, &freeSpace);

              if (SUCCEEDED(hrCap) && capacity > 0) {
                // MTP (Android/iOS) uses Decimal GB (1000^3)
                double totalGB = static_cast<double>(capacity) / 1000000000.0;
                double freeGB = static_cast<double>(freeSpace) / 1000000000.0;

                // Heuristic: Snap to Marketing Capacity (Standard Sizes: 32,
                // 64, 128, 256, 512, 1024)
                // Android System partition (~15GB) is hidden from MTP, causing
                // mismatch with Phone Settings. We project "Hidden System" by
                // assuming the device is a standard size.
                double standardSizes[] = {16, 32, 64, 128, 256, 512, 1024};
                for (double stdSize : standardSizes) {
                  // If detected size is 75%-100% of a standard size, assume
                  // it's that model
                  if (totalGB >= (stdSize * 0.75) &&
                      totalGB <= stdSize * 1.05) {
                    totalGB = stdSize;
                    break;
                  }
                }

                double usedGB = totalGB - freeGB;
                double usagePct =
                    (totalGB > 0) ? (usedGB / totalGB) * 100.0 : 0.0;

                QVariantMap drive;
                drive["name"] = name; // Mobile Name
                drive["mount"] = "MTP";
                drive["device"] = QString::fromWCharArray(deviceIds[i]);
                drive["fsType"] = "MTP";
                drive["total"] = QString::number(totalGB, 'f', 1);
                drive["used"] = QString::number(usedGB, 'f', 1);
                drive["free"] = QString::number(freeGB, 'f', 1);
                drive["usage"] = usagePct;
                drive["freePercent"] = 100.0 - usagePct;
                drive["hasUsage"] = true;
                drive["isMounted"] = true;
                devices.append(drive);
              }
            }
            CoTaskMemFree(objectIds[j]);
          }
        }
        device->Close();
      }
    }
    CoTaskMemFree(deviceIds[i]);
  }
  return devices;
}

void SystemStatsMonitor::readDiskUsage() {
  QVariantList drives;
  double totalAll = 0;
  double usedAll = 0;

  DWORD driveMask = GetLogicalDrives();
  for (char letter = 'A'; letter <= 'Z'; ++letter) {
    if (driveMask & 1) {
      QString rootPath = QString("%1:\\").arg(letter);

      UINT driveType = GetDriveTypeW((LPCWSTR)rootPath.utf16());

      bool isValidType =
          (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE ||
           driveType == DRIVE_REMOTE);

      if (isValidType) {
        WCHAR volumeName[MAX_PATH + 1];
        WCHAR fileSystemName[MAX_PATH + 1];
        DWORD serialNumber = 0;
        DWORD maxComponentLen = 0;
        DWORD fileSystemFlags = 0;

        BOOL getVolSuccess = GetVolumeInformationW(
            (LPCWSTR)rootPath.utf16(), volumeName, MAX_PATH, &serialNumber,
            &maxComponentLen, &fileSystemFlags, fileSystemName, MAX_PATH);

        QString label;
        QString fsType;
        if (getVolSuccess) {
          label = QString::fromWCharArray(volumeName);
          fsType = QString::fromWCharArray(fileSystemName);
        }

        ULARGE_INTEGER freeBytesAvailableToCaller;
        ULARGE_INTEGER totalNumberOfBytes;
        ULARGE_INTEGER totalNumberOfFreeBytes;

        BOOL getSpaceSuccess = GetDiskFreeSpaceExW(
            (LPCWSTR)rootPath.utf16(), &freeBytesAvailableToCaller,
            &totalNumberOfBytes, &totalNumberOfFreeBytes);

        if (getSpaceSuccess) {
          double totalGB = static_cast<double>(totalNumberOfBytes.QuadPart) /
                           (1024.0 * 1024.0 * 1024.0);
          double freeGB = static_cast<double>(totalNumberOfFreeBytes.QuadPart) /
                          (1024.0 * 1024.0 * 1024.0);
          double usedGB = totalGB - freeGB;

          if (totalNumberOfBytes.QuadPart < 100LL * 1024 * 1024) {
            driveMask >>= 1;
            continue;
          }

          double usagePct = (totalGB > 0) ? (usedGB / totalGB) * 100.0 : 0.0;

          QString displayName = label;
          if (displayName.isEmpty()) {
            displayName = QString("Local Disk (%1)").arg(rootPath.left(2));
          }

          QVariantMap drive;
          drive["name"] = displayName;
          drive["mount"] = rootPath;
          drive["device"] = rootPath;
          drive["fsType"] = fsType;

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
    }
    driveMask >>= 1;
  }

  for (const QVariant &v : m_cachedMtpDevices) {
    drives.append(v.toMap());
  }

  m_diskPartitions = drives;

  if (totalAll > 0) {
    m_diskUsage = (int)((usedAll / totalAll) * 100.0);
  } else {
    m_diskUsage = 0;
  }
}

// --- Charge Limit Logic ---

void SystemStatsMonitor::readChargeLimit() {
  // Read from Registry (Source of Truth)
  QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\ASUS\\ASUS System Control "
                    "Interface\\AsusOptimization\\ASUS Keyboard Hotkeys";
  QString keyName = "ChargingRate";

  QSettings settings(regPath, QSettings::NativeFormat);
  if (settings.status() == QSettings::NoError) {
    bool ok;
    int val = settings.value(keyName).toInt(&ok);
    if (ok && val >= 60 && val <= 100) {
      if (m_chargeLimit != val) {
        m_chargeLimit = val;
        m_pendingChargeLimit = val;
        emit chargeLimitChanged();
      }
    }
  }
}

void SystemStatsMonitor::setChargeLimit(int limit) {
  if (limit < 60)
    limit = 60;
  if (limit > 100)
    limit = 100;

  m_pendingChargeLimit = limit;
  m_chargeLimit = limit; // Optimistic update

  // Debounce actual WMI call
  m_limitDebounceTimer->start();

  emit chargeLimitChanged();
}

void SystemStatsMonitor::applyPendingChargeLimit() {
  // Logic matches Linux 'applyPendingChargeLimit'
  int limit = m_pendingChargeLimit;

  // 1. Validation
  if (limit < 60)
    limit = 60;
  if (limit > 100)
    limit = 100;

  // 2. Write to WMI (Primary Method)
  writeChargeLimitToWmi(limit);

  // 3. Verification Steps (Matching Linux robustness)
  // Read back to confirm? (Optional, but adds robustness)
  // int currentLimit = readChargeLimit(); // if implemented

  // 4. Persistence Config
  // Save to QSettings (mimics /etc/asus_battery_limit.conf)
  QSettings settings("AsusTuf", "FanControl");
  settings.setValue("ChargeLimit", limit);
  settings.sync(); // Force write to disk immediately

  // 5. Update Local State
  m_chargeLimit = limit;
  emit chargeLimitChanged();

  // 6. Force Enforcement just to be sure
  enforceChargeLimit();
}

void SystemStatsMonitor::enforceChargeLimit() {
  // Check Registry first to avoid unnecessary Service Restarts
  QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\ASUS\\ASUS System Control "
                    "Interface\\AsusOptimization\\ASUS Keyboard Hotkeys";
  QString keyName = "ChargingRate";

  QSettings settings(regPath, QSettings::NativeFormat);
  if (settings.status() == QSettings::NoError) {
    bool ok;
    int currentRegVal = settings.value(keyName).toInt(&ok);

    // If Registry matches our target, DO NOTHING.
    // This prevents restarting the service every 5 seconds.
    if (ok && currentRegVal == m_chargeLimit) {
      return;
    }
  }

  // Only write if there is a mismatch
  writeChargeLimitToWmi(m_chargeLimit);
}

void SystemStatsMonitor::writeChargeLimitToWmi(int limit) {
  // NEW METHOD: Write to Registry + Restart Service (Async/Non-blocking)

  QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\ASUS\\ASUS System Control "
                    "Interface\\AsusOptimization\\ASUS Keyboard Hotkeys";
  QString keyName = "ChargingRate";

  QSettings settings(regPath, QSettings::NativeFormat);
  if (settings.status() != QSettings::NoError) {
    emit errorOccurred("Failed to open Registry key.");
    return;
  }

  settings.setValue(keyName, limit);
  settings.sync();

  if (settings.status() != QSettings::NoError) {
    emit errorOccurred("Failed to write to Registry.");
    return;
  }

  // Restart Service Asynchronously (Fire and Forget) to prevent UI lag
  // Using QProcess::startDetached allows the command to run independent of the
  // app
  // Output redirected to NUL to silence logs
  QString program = "cmd.exe";
  QStringList arguments;
  arguments << "/c"
            << "net stop AsusOptimization > NUL 2>&1 && net start "
               "AsusOptimization > NUL 2>&1";

  // We don't wait for this. It takes 1-3 seconds, but the UI remains
  // responsive.
  QProcess::startDetached(program, arguments);
}

void SystemStatsMonitor::openFileManager(const QString &path) {
  QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void SystemStatsMonitor::openFileManager(const QString &mount,
                                         const QString &device) {
  Q_UNUSED(device);
  QDesktopServices::openUrl(QUrl::fromLocalFile(mount));
}

void SystemStatsMonitor::onMtpDevicesFound(QVariantList devices) {
  m_cachedMtpDevices = devices;
  // trigger stats update to merge list?
  // For now we just store it.
}

// --- Missing Linux Port Implementations ---

void SystemStatsMonitor::readGpuStats() {
  int pdhUsage = 0;
  int ecTemp = 0;

  // 1. PDH READOUT (Universal - AMD/Intel/NVIDIA)
  if (m_pdhQuery && m_pdhGpuCounter) {
    PdhCollectQueryData((PDH_HQUERY)m_pdhQuery);

    PDH_FMT_COUNTERVALUE_ITEM *pItems = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwItemCount = 0;

    // First call to get buffer size
    PdhGetFormattedCounterArray((PDH_HCOUNTER)m_pdhGpuCounter, PDH_FMT_DOUBLE,
                                &dwBufferSize, &dwItemCount, NULL);

    if (dwBufferSize > 0) {
      pItems = (PDH_FMT_COUNTERVALUE_ITEM *)malloc(dwBufferSize);
      if (PdhGetFormattedCounterArray((PDH_HCOUNTER)m_pdhGpuCounter,
                                      PDH_FMT_DOUBLE, &dwBufferSize,
                                      &dwItemCount, pItems) == ERROR_SUCCESS) {

        double maxUsage = 0;
        for (DWORD i = 0; i < dwItemCount; i++) {
          if (pItems[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            double val = pItems[i].FmtValue.doubleValue;
            if (val > maxUsage)
              maxUsage = val;
          }
        }
        pdhUsage = static_cast<int>(std::round(maxUsage));
      }
      free(pItems);
    }
  }

  // 1b. ASUS EC GPU TEMP (Universal Base)
  ecTemp = AsusWinIO::instance().getGpuTemp();

  // 4. Apply Updates Conditionally
  // If NVIDIA is present, we SKIP these updates and rely on
  // onGpuProcessFinished to prevent race conditions.
  bool hasNvidia = false;
  for (const auto &gpu : m_gpuModels) {
    if (gpu.contains("NVIDIA", Qt::CaseInsensitive)) {
      hasNvidia = true;
      break;
    }
  }

  if (!hasNvidia) {
    m_gpuUsage = pdhUsage;

    if (ecTemp > 0) {
      m_gpuTemp = ecTemp;
    } else {
      m_gpuTemp = 0;
    }
  }

  // 2. NVIDIA SMI CHECK (Tier 1 - Only for NVIDIA)
  if (!m_gpuProcess)
    return;
  if (m_gpuProcess->state() != QProcess::NotRunning)
    return;

  m_gpuProcess->start(
      "nvidia-smi",
      QStringList()
          << "--query-gpu=name,clocks.gr,utilization.gpu,temperature.gpu"
          << "--format=csv,noheader,nounits");
}

void SystemStatsMonitor::onGpuProcessFinished(int exitCode,
                                              QProcess::ExitStatus status) {
  Q_UNUSED(status);

  // If nvidia-smi failed or output empty, assume dGPU is OFF/Optimus Sleep
  bool dGpuActive = (exitCode == 0);
  QString output;
  if (dGpuActive) {
    output = m_gpuProcess->readAllStandardOutput().trimmed();
    if (output.isEmpty())
      dGpuActive = false;
  }

  if (!dGpuActive) {
    // GPU is likely OFF. Use QStringList iterator to safely remove NVIDIA
    // entries
    bool listChanged = false;
    QMutableStringListIterator i(m_gpuModels);
    while (i.hasNext()) {
      QString val = i.next();
      // Remove if it looks like an NVIDIA card (but keep generic if we want
      // fallback?) Usually "NVIDIA GeForce..."
      if (val.contains("NVIDIA", Qt::CaseInsensitive)) {
        i.remove();
        listChanged = true;
      }
    }

    if (listChanged) {
      emit gpuModelsChanged();
    }

    // Reset stats
    m_gpuFreq = 0;
    // Don't reset usage - let PDH fallback handle it (Fixes flicker)
    emit statsChanged();
    return;
  }

  QStringList parts = output.split(",");
  if (parts.length() >= 3) {
    QString gpuName = parts[0].trimmed();
    m_gpuFreq = parts[1].trimmed().toDouble();
    m_gpuUsage = parts[2].trimmed().toInt();

    // If SMI gives us temperature (we need to ask for it), use it.
    // For now, we didn't ask SMI for temp in the command line below.
    // Let's rely on ASUS EC for temp as it's consistent.
    // Or we could add temperature.gpu to the query?
    // User complaint: "task manager showing gpu temp 42 c but application
    // showing 0 c" If ASUS EC works, likely fixed.

    // DYNAMIC GPU DISCOVERY
    // If this GPU is not in our list, add it!
    if (!gpuName.isEmpty() && !m_gpuModels.contains(gpuName)) {
      // If we had a generic placeholder, remove it
      if (m_gpuModels.contains("Generic Graphics Adapter")) {
        m_gpuModels.removeAll("Generic Graphics Adapter");
      }

      m_gpuModels.append(gpuName);
      emit gpuModelsChanged(); // Notify UI to refresh list
    }

    // Read Temp if available (Index 3)
    if (parts.size() >= 4) {
      m_gpuTemp = parts[3].trimmed().toInt();
    } else {
      // Fallback to ASUS EC if nvidia-smi doesn't return temp
      int ecTemp = AsusWinIO::instance().getGpuTemp();
      if (ecTemp > 0)
        m_gpuTemp = ecTemp;
    }

  } else if (parts.length() >= 2) {
    // Fallback relative to old query
    m_gpuFreq = parts[0].trimmed().toDouble();
    m_gpuUsage = parts[1].trimmed().toInt();
  }
  emit statsChanged();
}

// Struct for Power Info if not fully defined in MinGW headers

void SystemStatsMonitor::readCpuFreq() {
  // Use PDH if available (Accurate Real-time via % of Max Frequency)
  if (m_pdhQuery && m_pdhCpuFreqCounter) {
    PDH_FMT_COUNTERVALUE displayValue;
    if (PdhGetFormattedCounterValue((PDH_HCOUNTER)m_pdhCpuFreqCounter,
                                    PDH_FMT_DOUBLE, NULL,
                                    &displayValue) == ERROR_SUCCESS) {
      if (displayValue.CStatus == PDH_CSTATUS_VALID_DATA) {
        // Value is Percentage (e.g. 100.0 or 140.0)
        // m_maxCpuMhz is base clock (e.g. 2500)
        if (m_maxCpuMhz > 0) {
          m_cpuFreq = (displayValue.doubleValue / 100.0) * m_maxCpuMhz;
        } else {
          m_cpuFreq = displayValue.doubleValue * 25.0; // Fallback
        }
        return;
      }
    }
  }

  // Fallback to CallNtPowerInformation (Legacy)
  int numCpus = QThread::idealThreadCount();
  if (numCpus <= 0)
    numCpus = 1;

  int size = numCpus * sizeof(PROCESSOR_POWER_INFORMATION_EX);
  std::vector<PROCESSOR_POWER_INFORMATION_EX> buffer(numCpus);

  long status = CallNtPowerInformation(static_cast<POWER_INFORMATION_LEVEL>(11),
                                       NULL, 0, buffer.data(), size);

  if (status == 0) { // STATUS_SUCCESS
    double totalFreq = 0;
    int activeCount = 0;

    for (const auto &info : buffer) {
      if (info.CurrentMhz > 0) {
        totalFreq += info.CurrentMhz;
        activeCount++;
      }
    }

    if (activeCount > 0) {
      m_cpuFreq = (totalFreq / activeCount);
    }
  } else {
    m_cpuFreq = 0.0;
  }
}
