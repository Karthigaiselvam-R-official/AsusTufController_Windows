#ifndef ASUSWINIO_H
#define ASUSWINIO_H

#include <iostream>
#include <windows.h>

class AsusWinIO {
public:
  static AsusWinIO &instance();
  bool initialize();

  // Fan Control
  void setFanIndex(unsigned char index);
  void setFanTestMode(bool enabled);
  void setFanPwmDuty(unsigned char duty); // 0-255

  // Reading Functions
  int getFanCounts();       // Number of controllable fans
  int getFanRPM(int index); // RPM for specific fan (0=CPU, 1=GPU)
  int getCpuTemp();         // CPU Temperature in Celsius

  // Status
  bool isInitialized() const { return m_initialized; }

private:
  AsusWinIO();
  ~AsusWinIO();

  HMODULE m_hModule;

  // Function Pointers - MATCHING KOAMEL'S SIGNATURES EXACTLY
  typedef void (*InitializeWinIoFn)(); // Returns VOID, not bool!
  typedef void (*ShutdownWinIoFn)();
  typedef void (*SetFanIndexFn)(unsigned char); // byte
  typedef void (*SetFanTestModeFn)(char);       // char
  typedef void (*SetFanPwmDutyFn)(short);       // SHORT, not char!
  typedef int (*GetFanRPMFn)();
  typedef int (*GetFanCountsFn)();
  typedef unsigned long long (*GetCpuTempFn)();

  InitializeWinIoFn m_initializeWinIo;
  ShutdownWinIoFn m_shutdownWinIo;
  SetFanIndexFn m_setFanIndex;
  SetFanTestModeFn m_setFanTestMode;
  SetFanPwmDutyFn m_setFanPwmDuty;
  GetFanRPMFn m_getFanRPM;
  GetFanCountsFn m_getFanCounts;
  GetCpuTempFn m_getCpuTemp;

  bool m_initialized;
};

#endif // ASUSWINIO_H
