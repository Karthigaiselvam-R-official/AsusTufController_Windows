#ifndef ASUSWINIO_H
#define ASUSWINIO_H

#include <windows.h>
#include <iostream>

class AsusWinIO {
public:
    static AsusWinIO& instance();
    bool initialize();
    
    // Fan Control
    void setFanIndex(unsigned char index);
    void setFanTestMode(bool enabled);
    void setFanPwmDuty(unsigned char duty); // 0-255
    int getFanRPM();

private:
    AsusWinIO();
    ~AsusWinIO();

    HMODULE m_hModule;
    
    // Function Pointers
    typedef bool (*InitializeWinIoFn)();
    typedef void (*SetFanIndexFn)(char);
    typedef void (*SetFanTestModeFn)(char);
    typedef void (*SetFanPwmDutyFn)(char);
    typedef int (*GetFanRPMFn)();

    InitializeWinIoFn m_initializeWinIo;
    SetFanIndexFn m_setFanIndex;
    SetFanTestModeFn m_setFanTestMode;
    SetFanPwmDutyFn m_setFanPwmDuty;
    GetFanRPMFn m_getFanRPM;

    bool m_initialized;
};

#endif // ASUSWINIO_H
