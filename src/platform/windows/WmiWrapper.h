#ifndef WMIWRAPPER_H
#define WMIWRAPPER_H

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <string>

#pragma comment(lib, "wbemuuid.lib")

class WmiWrapper {
public:
    static WmiWrapper& instance();
    bool initialize();
    
    // Generic ASUS WMI Method
    // Used for Fan Policy, Battery Limit, etc.
    // Returns true on success. 'result' is optional output.
    bool setDevice(DWORD deviceId, DWORD value, DWORD* result = nullptr);
    
    // For reading sensors if needed (CPU Temp might be via different method)
    // Note: Standard temperature reading might need a different WMI class like MSAcpi_ThermalZoneTemperature
    
private:
    WmiWrapper();
    ~WmiWrapper();

    bool m_initialized;
    IWbemLocator* m_pLoc;
    IWbemServices* m_pSvc;
};

#endif // WMIWRAPPER_H
