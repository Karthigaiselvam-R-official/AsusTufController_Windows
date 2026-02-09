#include "AuraHID.h"

// Minimal Windows include - only what we absolutely need
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// We dynamically load ALL HID-related functions to avoid header conflicts
typedef struct _HIDD_ATTRIBUTES_LOCAL {
  ULONG Size;
  USHORT VendorID;
  USHORT ProductID;
  USHORT VersionNumber;
} HIDD_ATTRIBUTES_LOCAL;

// Function pointer types
typedef void(WINAPI *pfnHidD_GetHidGuid)(GUID *);
typedef BOOLEAN(WINAPI *pfnHidD_GetAttributes)(HANDLE, HIDD_ATTRIBUTES_LOCAL *);
typedef BOOLEAN(WINAPI *pfnHidD_SetFeature)(HANDLE, PVOID, ULONG);

// Globals for DLL functions
static HMODULE g_hidDll = nullptr;
static pfnHidD_GetHidGuid g_HidD_GetHidGuid = nullptr;
static pfnHidD_GetAttributes g_HidD_GetAttributes = nullptr;
static pfnHidD_SetFeature g_HidD_SetFeature = nullptr;

// GUID for HID class (constant)
static GUID g_HidGuid = {0};

// Message length for USB HID reports
static const size_t MESSAGE_LENGTH = 65;

// Known ASUS Keyboard Product IDs
static const USHORT ASUS_KEYBOARD_PIDS[] = {
    // TUF Gaming Series
    0x1854,
    0x1866,
    0x1869,
    0x1907,
    0x1914,
    0x193B,
    0x1A30,
    // ROG Strix Series
    0x1816,
    0x1817,
    0x1834,
    0x1837,
    0x189C,
    0x1932,
    0x19B6,
    0x1A13,
    0x1A5E,
    // ROG Zephyrus Series
    0x1845,
    0x1846,
    0x1855,
    0x1856,
    // Additional
    0x18C6,
    0x19AF,
    0x1822,
    0x1837,
};
static const size_t NUM_ASUS_PIDS =
    sizeof(ASUS_KEYBOARD_PIDS) / sizeof(ASUS_KEYBOARD_PIDS[0]);

// Load HID DLL and functions
static bool loadHidDll() {
  if (g_hidDll)
    return true;

  g_hidDll = LoadLibraryW(L"hid.dll");
  if (!g_hidDll)
    return false;

  g_HidD_GetHidGuid =
      (pfnHidD_GetHidGuid)GetProcAddress(g_hidDll, "HidD_GetHidGuid");
  g_HidD_GetAttributes =
      (pfnHidD_GetAttributes)GetProcAddress(g_hidDll, "HidD_GetAttributes");
  g_HidD_SetFeature =
      (pfnHidD_SetFeature)GetProcAddress(g_hidDll, "HidD_SetFeature");

  if (!g_HidD_GetHidGuid || !g_HidD_GetAttributes || !g_HidD_SetFeature) {
    FreeLibrary(g_hidDll);
    g_hidDll = nullptr;
    return false;
  }

  // Get the HID GUID
  g_HidD_GetHidGuid(&g_HidGuid);
  return true;
}

// Convert GUID to registry path string
static void guidToString(const GUID &guid, wchar_t *output) {
  wsprintfW(output, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
            guid.Data4[6], guid.Data4[7]);
}

AuraHID &AuraHID::instance() {
  static AuraHID inst;
  return inst;
}

AuraHID::AuraHID()
    : m_deviceHandle(nullptr), m_productId(0), m_initialized(false) {}

AuraHID::~AuraHID() { shutdown(); }

bool AuraHID::isConnected() const {
  return m_deviceHandle != nullptr && m_deviceHandle != INVALID_HANDLE_VALUE;
}

unsigned short AuraHID::getProductId() const { return m_productId; }

bool AuraHID::initialize() {
  if (m_initialized && isConnected()) {
    return true;
  }

  if (!loadHidDll()) {
    return false;
  }

  if (findAsusKeyboard()) {
    m_initialized = true;
    initializeKeyboard();
    return true;
  }

  return false;
}

void AuraHID::shutdown() {
  closeDevice();
  m_initialized = false;
}

bool AuraHID::findAsusKeyboard() {
  // Enumerate HID devices by checking well-known device path patterns
  // ASUS keyboards typically have paths like:
  // \\?\hid#vid_0b05&pid_XXXX#...

  wchar_t devicePath[MAX_PATH];

  // Try common ASUS keyboard PIDs
  for (size_t pidIdx = 0; pidIdx < NUM_ASUS_PIDS; pidIdx++) {
    USHORT pid = ASUS_KEYBOARD_PIDS[pidIdx];

    // Try multiple interface numbers (keyboards often have multiple interfaces)
    for (int iface = 0; iface < 4; iface++) {
      // Build device path pattern
      wsprintfW(devicePath, L"\\\\?\\hid#vid_0b05&pid_%04x&mi_%02x", pid,
                iface);

      // Try to find matching device using FindFirstFile on device namespace
      // This is a simplified approach - in production we'd use SetupDI
      wchar_t searchPath[MAX_PATH];
      wsprintfW(searchPath, L"\\\\.\\HID#VID_0B05&PID_%04X", pid);

      // Try direct open with basic path variations
      wchar_t testPaths[3][MAX_PATH];
      wsprintfW(testPaths[0], L"\\\\?\\HID#VID_0B05&PID_%04X&MI_02#", pid);
      wsprintfW(testPaths[1], L"\\\\?\\HID#VID_0B05&PID_%04X&MI_00#", pid);
      wsprintfW(testPaths[2], L"\\\\?\\HID#VID_0B05&PID_%04X#", pid);

      // We need to use registry enumeration instead
      HKEY hKey;
      wchar_t regPath[512];
      wsprintfW(regPath,
                L"SYSTEM\\CurrentControlSet\\Enum\\HID\\VID_0B05&PID_%04X",
                pid);

      if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) ==
          ERROR_SUCCESS) {
        // Found a matching device class in registry
        wchar_t subKeyName[256];
        DWORD subKeyIndex = 0;
        DWORD subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t);

        while (RegEnumKeyExW(hKey, subKeyIndex++, subKeyName, &subKeyNameLen,
                             NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
          // Check if this device instance has a DeviceParameters key
          HKEY hDevKey;
          wchar_t devPath[512];
          wsprintfW(devPath, L"%ls\\%ls\\Device Parameters", regPath,
                    subKeyName);

          if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, devPath, 0, KEY_READ,
                            &hDevKey) == ERROR_SUCCESS) {
            // Try to get the SymbolicLink value
            wchar_t symLink[MAX_PATH] = {0};
            DWORD symLinkSize = sizeof(symLink);

            // Build device path from registry info
            wchar_t fullDevPath[512];
            wsprintfW(fullDevPath,
                      L"\\\\?\\HID#VID_0B05&PID_%04X#%ls#{4d1e55b2-f16f-11cf-"
                      L"88cb-001111000030}",
                      pid, subKeyName);

            // Try to open this device
            HANDLE hDevice =
                CreateFileW(fullDevPath, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);

            if (hDevice != INVALID_HANDLE_VALUE) {
              // Verify it's an ASUS device
              HIDD_ATTRIBUTES_LOCAL attrs;
              attrs.Size = sizeof(attrs);

              if (g_HidD_GetAttributes(hDevice, &attrs)) {
                if (attrs.VendorID == ASUS_USB_VENDOR_ID) {
                  m_deviceHandle = hDevice;
                  m_productId = attrs.ProductID;
                  RegCloseKey(hDevKey);
                  RegCloseKey(hKey);
                  return true;
                }
              }
              CloseHandle(hDevice);
            }
            RegCloseKey(hDevKey);
          }
          subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t);
        }
        RegCloseKey(hKey);
      }
    }
  }

  return false;
}

void AuraHID::closeDevice() {
  if (m_deviceHandle && m_deviceHandle != INVALID_HANDLE_VALUE) {
    CloseHandle((HANDLE)m_deviceHandle);
    m_deviceHandle = nullptr;
  }
  m_productId = 0;
}

bool AuraHID::sendFeatureReport(const unsigned char *data, size_t length) {
  if (!isConnected() || !g_HidD_SetFeature) {
    return false;
  }

  // Prepare buffer with padding
  unsigned char buffer[MESSAGE_LENGTH];
  memset(buffer, 0, MESSAGE_LENGTH);
  size_t copyLen = (length < MESSAGE_LENGTH) ? length : MESSAGE_LENGTH;
  memcpy(buffer, data, copyLen);

  // Try HID feature report
  if (g_HidD_SetFeature((HANDLE)m_deviceHandle, buffer, MESSAGE_LENGTH)) {
    return true;
  }

  // Fallback: try WriteFile
  DWORD bytesWritten = 0;
  if (WriteFile((HANDLE)m_deviceHandle, buffer, MESSAGE_LENGTH, &bytesWritten,
                NULL)) {
    return bytesWritten > 0;
  }

  return false;
}

bool AuraHID::setMode(AuraMode mode, const AuraColor &color, AuraSpeed speed) {
  if (!m_initialized && !initialize()) {
    return false;
  }

  // Build message: [ReportID] 5D B3 00 <mode> <R> <G> <B> <speed>
  unsigned char message[MESSAGE_LENGTH];
  memset(message, 0, MESSAGE_LENGTH);

  message[0] = 0x00; // Report ID
  message[1] = 0x5D;
  message[2] = 0xB3;
  message[3] = 0x00; // Zone (0 = all)
  message[4] = static_cast<unsigned char>(mode);
  message[5] = color.r;
  message[6] = color.g;
  message[7] = color.b;
  message[8] = static_cast<unsigned char>(speed);

  if (!sendFeatureReport(message, MESSAGE_LENGTH)) {
    return false;
  }

  return apply();
}

bool AuraHID::setBrightness(int level) {
  if (!m_initialized && !initialize()) {
    return false;
  }

  // Clamp to 0-3
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;

  // Build brightness message: [ReportID] 5A BA C5 C4 <level>
  unsigned char message[MESSAGE_LENGTH];
  memset(message, 0, MESSAGE_LENGTH);

  message[0] = 0x00;
  message[1] = 0x5A;
  message[2] = 0xBA;
  message[3] = 0xC5;
  message[4] = 0xC4;
  message[5] = static_cast<unsigned char>(level);

  return sendFeatureReport(message, MESSAGE_LENGTH);
}

bool AuraHID::initializeKeyboard() {
  if (!isConnected()) {
    return false;
  }

  // Send initialization message: "ASUS Tech.Inc."
  unsigned char message[MESSAGE_LENGTH];
  memset(message, 0, MESSAGE_LENGTH);

  const unsigned char initMsg[] = {0x00, 0x5A, 0x41, 0x53, 0x55, 0x53,
                                   0x20, 0x54, 0x65, 0x63, 0x68, 0x2E,
                                   0x49, 0x6E, 0x63, 0x2E, 0x00};
  memcpy(message, initMsg, sizeof(initMsg));

  return sendFeatureReport(message, MESSAGE_LENGTH);
}

bool AuraHID::apply() {
  if (!isConnected()) {
    return false;
  }

  // Send apply message: [ReportID] 5D B4
  unsigned char message[MESSAGE_LENGTH];
  memset(message, 0, MESSAGE_LENGTH);

  message[0] = 0x00;
  message[1] = 0x5D;
  message[2] = 0xB4;

  return sendFeatureReport(message, MESSAGE_LENGTH);
}
