#ifndef WMIWRAPPER_H
#define WMIWRAPPER_H

#include <Wbemidl.h>
#include <comdef.h>
#include <string>
#include <windows.h>


#pragma comment(lib, "wbemuuid.lib")

// ASUS WMI Device IDs (from asus-wmi Linux driver)
#define ASUS_WMI_DEVID_THROTTLE_THERMAL_POLICY 0x00120075
#define ASUS_WMI_DEVID_FAN_BOOST_MODE 0x00110018
#define ASUS_WMI_DEVID_CPU_FAN_CURVE 0x00110024
#define ASUS_WMI_DEVID_GPU_FAN_CURVE 0x00110025
#define ASUS_WMI_DEVID_CPU_FAN_CTRL 0x00110013 // Manual fan control
#define ASUS_WMI_DEVID_GPU_FAN_CTRL 0x00110014

// Thermal policy values
#define ASUS_THERMAL_POLICY_SILENT 0
#define ASUS_THERMAL_POLICY_BALANCED 1
#define ASUS_THERMAL_POLICY_TURBO 2

class WmiWrapper {
public:
  static WmiWrapper &instance();
  bool initialize();

  // Fan control methods
  bool setThermalPolicy(int policy); // 0=Silent, 1=Balanced, 2=Turbo
  int getThermalPolicy();

  // Direct fan control (0-255 PWM duty)
  bool setCpuFanDuty(unsigned char duty);
  bool setGpuFanDuty(unsigned char duty);

  // Temperature reading
  int getCpuTemperature();

  // Generic ASUS WMI Method
  bool setDevice(DWORD deviceId, DWORD value, DWORD *result = nullptr);
  bool getDevice(DWORD deviceId, DWORD *result);

  bool isInitialized() const { return m_initialized; }

private:
  WmiWrapper();
  ~WmiWrapper();

  bool executeWmiMethod(const wchar_t *methodName, DWORD arg0, DWORD arg1,
                        DWORD *result);

  bool m_initialized;
  IWbemLocator *m_pLoc;
  IWbemServices *m_pSvc;
  IWbemClassObject *m_pClass;
  std::wstring m_instanceName;
};

#endif // WMIWRAPPER_H
