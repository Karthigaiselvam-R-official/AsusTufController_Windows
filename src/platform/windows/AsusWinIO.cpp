#include "AsusWinIO.h"

AsusWinIO& AsusWinIO::instance() {
    static AsusWinIO instance;
    return instance;
}

AsusWinIO::AsusWinIO() : m_hModule(nullptr), m_initialized(false) {}

AsusWinIO::~AsusWinIO() {
    if (m_hModule) {
        FreeLibrary(m_hModule);
    }
}

bool AsusWinIO::initialize() {
    if (m_initialized) return true;

    // Load the DLL
    m_hModule = LoadLibraryA("AsusWinIO64.dll");
    if (!m_hModule) {
        std::cerr << "Failed to load AsusWinIO64.dll" << std::endl;
        return false;
    }

    // Map functions
    m_initializeWinIo = (InitializeWinIoFn)GetProcAddress(m_hModule, "InitializeWinIo");
    // "HealthyTable_" prefix is used in the C# import, checking if it's the same in DLL export names.
    // Assuming the C# P/Invoke name matches the export name. 
    // If not, we might need to inspect the DLL exports, but usually they match.
    m_setFanIndex = (SetFanIndexFn)GetProcAddress(m_hModule, "HealthyTable_SetFanIndex");
    m_setFanTestMode = (SetFanTestModeFn)GetProcAddress(m_hModule, "HealthyTable_SetFanTestMode");
    m_setFanPwmDuty = (SetFanPwmDutyFn)GetProcAddress(m_hModule, "HealthyTable_SetFanPwmDuty");
    m_getFanRPM = (GetFanRPMFn)GetProcAddress(m_hModule, "HealthyTable_FanRPM"); // Guessing name

    if (!m_initializeWinIo || !m_setFanIndex || !m_setFanTestMode || !m_setFanPwmDuty) {
        std::cerr << "Failed to map all functions from AsusWinIO64.dll" << std::endl;
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
        return false;
    }

    // Initialize the driver logic
    if (!m_initializeWinIo()) {
        std::cerr << "InitializeWinIo returned false" << std::endl;
        return false;
    }

    m_initialized = true;
    return true;
}

void AsusWinIO::setFanIndex(unsigned char index) {
    if (m_initialized && m_setFanIndex) {
        m_setFanIndex((char)index);
    }
}

void AsusWinIO::setFanTestMode(bool enabled) {
    if (m_initialized && m_setFanTestMode) {
        // C# used (char)(value > 0 ? 0x01 : 0x00)
        m_setFanTestMode(enabled ? 0x01 : 0x00);
    }
}

void AsusWinIO::setFanPwmDuty(unsigned char duty) {
    if (m_initialized && m_setFanPwmDuty) {
        m_setFanPwmDuty((char)duty);
    }
}

int AsusWinIO::getFanRPM() {
    if (m_initialized && m_getFanRPM) {
        return m_getFanRPM();
    }
    return 0;
}
