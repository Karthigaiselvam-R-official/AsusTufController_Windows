#include "AsusWinIO.h"

AsusWinIO &AsusWinIO::instance() {
  static AsusWinIO instance;
  return instance;
}

AsusWinIO::AsusWinIO()
    : m_hModule(nullptr), m_initialized(false), m_initializeWinIo(nullptr),
      m_shutdownWinIo(nullptr), m_setFanIndex(nullptr),
      m_setFanTestMode(nullptr), m_setFanPwmDuty(nullptr), m_getFanRPM(nullptr),
      m_getFanCounts(nullptr), m_getCpuTemp(nullptr) {}

AsusWinIO::~AsusWinIO() {
  // Call ShutdownWinIo before unloading
  if (m_initialized && m_shutdownWinIo) {
    m_shutdownWinIo();
  }
  if (m_hModule) {
    FreeLibrary(m_hModule);
  }
}

bool AsusWinIO::initialize() {
  if (m_initialized)
    return true;

  // Load the DLL
  m_hModule = LoadLibraryA("AsusWinIO64.dll");
  if (!m_hModule) {
    std::cerr << "Failed to load AsusWinIO64.dll" << std::endl;
    return false;
  }

  // Map functions - EXACTLY as Koamel does
  m_initializeWinIo =
      (InitializeWinIoFn)GetProcAddress(m_hModule, "InitializeWinIo");
  m_shutdownWinIo = (ShutdownWinIoFn)GetProcAddress(m_hModule, "ShutdownWinIo");
  m_setFanIndex =
      (SetFanIndexFn)GetProcAddress(m_hModule, "HealthyTable_SetFanIndex");
  m_setFanTestMode = (SetFanTestModeFn)GetProcAddress(
      m_hModule, "HealthyTable_SetFanTestMode");
  m_setFanPwmDuty =
      (SetFanPwmDutyFn)GetProcAddress(m_hModule, "HealthyTable_SetFanPwmDuty");
  m_getFanRPM = (GetFanRPMFn)GetProcAddress(m_hModule, "HealthyTable_FanRPM");
  m_getFanCounts =
      (GetFanCountsFn)GetProcAddress(m_hModule, "HealthyTable_FanCounts");
  m_getCpuTemp =
      (GetCpuTempFn)GetProcAddress(m_hModule, "Thermal_Read_Cpu_Temperature");

  if (!m_initializeWinIo || !m_setFanIndex || !m_setFanTestMode ||
      !m_setFanPwmDuty) {
    std::cerr << "Failed to map all functions from AsusWinIO64.dll"
              << std::endl;
    FreeLibrary(m_hModule);
    m_hModule = nullptr;
    return false;
  }

  // Initialize the driver
  m_initializeWinIo();
  // Let driver initialize
  Sleep(500);

  // Get fan count
  int fanCount = 0;
  if (m_getFanCounts) {
    fanCount = m_getFanCounts();
  }

  m_initialized = true;
  return true;
}

void AsusWinIO::setFanIndex(unsigned char index) {
  if (m_initialized && m_setFanIndex) {
    m_setFanIndex(index);
  }
}

void AsusWinIO::setFanTestMode(bool enabled) {
  if (m_initialized && m_setFanTestMode) {
    char mode = enabled ? 0x01 : 0x00;
    m_setFanTestMode(mode);
  }
}

void AsusWinIO::setFanPwmDuty(unsigned char duty) {
  if (m_initialized && m_setFanPwmDuty) {
    // IMPORTANT: Koamel uses SHORT, not char!
    m_setFanPwmDuty(static_cast<short>(duty));
  }
}

int AsusWinIO::getFanCounts() {
  if (m_initialized && m_getFanCounts) {
    return m_getFanCounts();
  }
  return 2; // Default to 2 fans
}

int AsusWinIO::getFanRPM(int index) {
  if (m_initialized && m_getFanRPM) {
    setFanIndex(static_cast<unsigned char>(index));
    return m_getFanRPM();
  }
  return 0;
}

int AsusWinIO::getCpuTemp() {
  if (m_initialized && m_getCpuTemp) {
    return static_cast<int>(m_getCpuTemp());
  }
  return 0;
}
