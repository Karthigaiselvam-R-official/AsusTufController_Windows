#include "WmiWrapper.h"
#include <comdef.h>
#include <iostream>

WmiWrapper &WmiWrapper::instance() {
  static WmiWrapper instance;
  return instance;
}

WmiWrapper::WmiWrapper()
    : m_initialized(false), m_pLoc(nullptr), m_pSvc(nullptr),
      m_pClass(nullptr) {}

WmiWrapper::~WmiWrapper() {
  if (m_pClass)
    m_pClass->Release();
  if (m_pSvc)
    m_pSvc->Release();
  if (m_pLoc)
    m_pLoc->Release();
  if (m_initialized)
    CoUninitialize();
}

bool WmiWrapper::initialize() {
  if (m_initialized)
    return true;

  HRESULT hres;

  // 1. Initialize COM
  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres) && hres != RPC_E_CHANGED_MODE) {
    std::cerr << "WMI: Failed to initialize COM. Error: 0x" << std::hex << hres
              << std::endl;
  }

  // 2. Initialize Security
  hres =
      CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
  // Ignore if already set

  // 3. Obtain the WMI locator
  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *)&m_pLoc);

  if (FAILED(hres)) {
    std::cerr << "WMI: Failed to create IWbemLocator. Error: 0x" << std::hex
              << hres << std::endl;
    return false;
  }

  // 4. Connect to WMI in root\wmi namespace (where ASUS WMI lives)
  hres = m_pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0,
                               &m_pSvc);

  if (FAILED(hres)) {
    std::cerr << "WMI: Could not connect to root\\wmi. Error: 0x" << std::hex
              << hres << std::endl;
    m_pLoc->Release();
    m_pLoc = nullptr;
    return false;
  }

  // 5. Set security levels on the proxy
  hres = CoSetProxyBlanket(m_pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE);

  if (FAILED(hres)) {
    std::cerr << "WMI: Could not set proxy blanket. Error: 0x" << std::hex
              << hres << std::endl;
    m_pSvc->Release();
    m_pSvc = nullptr;
    m_pLoc->Release();
    m_pLoc = nullptr;
    return false;
  }

  // 6. Get the AsusAtkWmi_WMNB class
  hres =
      m_pSvc->GetObject(_bstr_t(L"AsusAtkWmi_WMNB"), 0, NULL, &m_pClass, NULL);

  if (FAILED(hres)) {
    std::cerr << "WMI: Could not get AsusAtkWmi_WMNB class. Error: 0x"
              << std::hex << hres << std::endl;
    // Continue anyway - we might still work with some fallbacks
  }

  // 7. Get the instance name by querying instances
  IEnumWbemClassObject *pEnumerator = nullptr;
  hres = m_pSvc->ExecQuery(
      _bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM AsusAtkWmi_WMNB"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
      &pEnumerator);

  if (SUCCEEDED(hres) && pEnumerator) {
    IWbemClassObject *pInstance = nullptr;
    ULONG uReturn = 0;

    if (pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &uReturn) == S_OK &&
        uReturn > 0) {
      VARIANT vtProp;
      hres = pInstance->Get(L"InstanceName", 0, &vtProp, 0, 0);
      if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
        m_instanceName = vtProp.bstrVal;
      }
      VariantClear(&vtProp);
      pInstance->Release();
    }
    pEnumerator->Release();
  }

  if (m_instanceName.empty()) {
    // WMI instance not found - this is okay, we use DLL control
  }

  m_initialized = true;
  return true;
}

bool WmiWrapper::executeWmiMethod(const wchar_t *methodName, DWORD arg0,
                                  DWORD arg1, DWORD *result) {
  if (!m_initialized || !m_pSvc || m_instanceName.empty()) {
    return false;
  }

  HRESULT hres;

  // Get method input parameters class
  IWbemClassObject *pClass = nullptr;
  hres = m_pSvc->GetObject(_bstr_t(L"AsusAtkWmi_WMNB"), 0, NULL, &pClass, NULL);
  if (FAILED(hres))
    return false;

  IWbemClassObject *pInParamsDefinition = nullptr;
  hres = pClass->GetMethod(methodName, 0, &pInParamsDefinition, NULL);
  pClass->Release();
  if (FAILED(hres))
    return false;

  // Create instance of input parameters
  IWbemClassObject *pInParams = nullptr;
  hres = pInParamsDefinition->SpawnInstance(0, &pInParams);
  pInParamsDefinition->Release();
  if (FAILED(hres))
    return false;

  // Set the input parameters
  VARIANT varArg0, varArg1;
  VariantInit(&varArg0);
  VariantInit(&varArg1);

  varArg0.vt = VT_I4;
  varArg0.lVal = static_cast<LONG>(arg0);
  varArg1.vt = VT_I4;
  varArg1.lVal = static_cast<LONG>(arg1);

  // For DEVS/DSTS methods, parameters are typically named differently
  // Let's try common ASUS WMI parameter names
  pInParams->Put(L"Data0", 0, &varArg0, 0); // Device ID
  pInParams->Put(L"Data1", 0, &varArg1, 0); // Value

  // Build the object path with instance name
  std::wstring objectPath = L"AsusAtkWmi_WMNB.InstanceName='";
  objectPath += m_instanceName;
  objectPath += L"'";

  // Execute the method
  IWbemClassObject *pOutParams = nullptr;
  hres = m_pSvc->ExecMethod(_bstr_t(objectPath.c_str()), _bstr_t(methodName), 0,
                            NULL, pInParams, &pOutParams, NULL);

  pInParams->Release();
  VariantClear(&varArg0);
  VariantClear(&varArg1);

  if (FAILED(hres)) {
    // WMI method failed - silently ignore, DLL control works
    return false;
  }

  // Get the result if requested
  if (result && pOutParams) {
    VARIANT varResult;
    VariantInit(&varResult);
    hres = pOutParams->Get(L"Data", 0, &varResult, NULL, NULL);
    if (SUCCEEDED(hres)) {
      *result = static_cast<DWORD>(varResult.lVal);
    }
    VariantClear(&varResult);
  }

  if (pOutParams)
    pOutParams->Release();
  return true;
}

bool WmiWrapper::setDevice(DWORD deviceId, DWORD value, DWORD *result) {
  // DEVS = Device Set
  return executeWmiMethod(L"DEVS", deviceId, value, result);
}

bool WmiWrapper::getDevice(DWORD deviceId, DWORD *result) {
  // DSTS = Device Status
  return executeWmiMethod(L"DSTS", deviceId, 0, result);
}

bool WmiWrapper::setThermalPolicy(int policy) {
  // Clamp to valid range
  if (policy < 0)
    policy = 0;
  if (policy > 2)
    policy = 2;

  DWORD result = 0;
  bool success = setDevice(ASUS_WMI_DEVID_THROTTLE_THERMAL_POLICY,
                           static_cast<DWORD>(policy), &result);

  return success;
}

int WmiWrapper::getThermalPolicy() {
  DWORD result = 0;
  if (getDevice(ASUS_WMI_DEVID_THROTTLE_THERMAL_POLICY, &result)) {
    return static_cast<int>(result & 0xFF);
  }
  return -1;
}

bool WmiWrapper::setCpuFanDuty(unsigned char duty) {
  // ASUS WMI fan control: value format is usually (duty << 16) | something
  // Or just duty directly. Let's try direct first.
  DWORD value = static_cast<DWORD>(duty);
  return setDevice(ASUS_WMI_DEVID_CPU_FAN_CTRL, value, nullptr);
}

bool WmiWrapper::setGpuFanDuty(unsigned char duty) {
  DWORD value = static_cast<DWORD>(duty);
  return setDevice(ASUS_WMI_DEVID_GPU_FAN_CTRL, value, nullptr);
}

int WmiWrapper::getCpuTemperature() {
  // Try to read temperature from MSAcpi_ThermalZoneTemperature
  HRESULT hres;
  IEnumWbemClassObject *pEnumerator = nullptr;

  // Connect to root\wmi for thermal zones
  hres = m_pSvc->ExecQuery(
      _bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM MSAcpi_ThermalZoneTemperature"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
      &pEnumerator);

  if (FAILED(hres) || !pEnumerator) {
    return 0;
  }

  IWbemClassObject *pInstance = nullptr;
  ULONG uReturn = 0;
  int temperature = 0;

  // Get the first thermal zone (usually CPU)
  if (pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &uReturn) == S_OK &&
      uReturn > 0) {
    VARIANT vtProp;
    hres = pInstance->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hres)) {
      // Temperature is in tenths of Kelvin, convert to Celsius
      // Formula: (value / 10) - 273.15
      temperature = static_cast<int>((vtProp.lVal / 10.0) - 273.15);
    }
    VariantClear(&vtProp);
    pInstance->Release();
  }

  pEnumerator->Release();
  return temperature;
}
