#include "AtkAcpi.h"
#include <cstring>
#include <windows.h>

static const wchar_t *ATK_DEVICE_PATH = L"\\\\.\\ATKACPI";

AtkAcpi &AtkAcpi::instance() {
  static AtkAcpi inst;
  return inst;
}

AtkAcpi::AtkAcpi() : m_handle(nullptr), m_initialized(false) {}
AtkAcpi::~AtkAcpi() { shutdown(); }

bool AtkAcpi::initialize() {
  if (m_initialized && isConnected())
    return true;

  HANDLE handle = CreateFileW(ATK_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE)
    return false;

  m_handle = handle;
  m_initialized = true;
  return true;
}

void AtkAcpi::shutdown() {
  if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
    CloseHandle((HANDLE)m_handle);
    m_handle = nullptr;
  }
  m_initialized = false;
}

bool AtkAcpi::isConnected() const {
  return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
}

bool AtkAcpi::callMethod(unsigned int methodId, unsigned char *args, int argLen,
                         unsigned char *outBuffer, int outLen) {
  if (!isConnected())
    return false;

  int bufferSize = 8 + argLen;
  unsigned char *acpiBuf = new unsigned char[bufferSize];
  memset(acpiBuf, 0, bufferSize);

  memcpy(acpiBuf, &methodId, 4);
  unsigned int argsLength = static_cast<unsigned int>(argLen);
  memcpy(acpiBuf + 4, &argsLength, 4);

  if (argLen > 0 && args)
    memcpy(acpiBuf + 8, args, argLen);

  DWORD bytesReturned = 0;
  BOOL result =
      DeviceIoControl((HANDLE)m_handle, ATK_CONTROL_CODE, acpiBuf, bufferSize,
                      outBuffer, outLen, &bytesReturned, nullptr);
  delete[] acpiBuf;
  return result != 0;
}

int AtkAcpi::deviceSet(unsigned int deviceId, int value) {
  unsigned char args[8];
  memset(args, 0, sizeof(args));
  memcpy(args, &deviceId, 4);
  memcpy(args + 4, &value, 4);

  unsigned char outBuffer[16];
  memset(outBuffer, 0, sizeof(outBuffer));

  if (!callMethod(ATK_DEVS, args, sizeof(args), outBuffer, sizeof(outBuffer)))
    return -1;

  int result = 0;
  memcpy(&result, outBuffer, 4);
  return result;
}

int AtkAcpi::deviceSet(unsigned int deviceId, unsigned char *params,
                       int paramLen) {
  int argsLen = 4 + paramLen;
  unsigned char *args = new unsigned char[argsLen];
  memset(args, 0, argsLen);
  memcpy(args, &deviceId, 4);
  if (paramLen > 0 && params)
    memcpy(args + 4, params, paramLen);

  unsigned char outBuffer[16];
  memset(outBuffer, 0, sizeof(outBuffer));

  bool success =
      callMethod(ATK_DEVS, args, argsLen, outBuffer, sizeof(outBuffer));
  delete[] args;
  if (!success)
    return -1;

  int result = 0;
  memcpy(&result, outBuffer, 4);
  return result;
}

int AtkAcpi::deviceGet(unsigned int deviceId) {
  unsigned char args[8];
  memset(args, 0, sizeof(args));
  memcpy(args, &deviceId, 4);

  unsigned char outBuffer[16];
  memset(outBuffer, 0, sizeof(outBuffer));

  if (!callMethod(ATK_DSTS, args, sizeof(args), outBuffer, sizeof(outBuffer)))
    return -1;

  int result = 0;
  memcpy(&result, outBuffer, 4);
  return result - 65536;
}

bool AtkAcpi::setTufKeyboardRGB(TufRgbMode mode, const TufColor &color,
                                int speed) {
  if (!m_initialized && !initialize())
    return false;

  unsigned char setting[6];
  setting[0] = 0xB4;
  setting[1] = static_cast<unsigned char>(mode);
  setting[2] = color.r;
  setting[3] = color.g;
  setting[4] = color.b;
  setting[5] = static_cast<unsigned char>(speed);

  int result = deviceSet(TUF_KB, setting, sizeof(setting));

  if (result != 1) {
    setting[0] = 0xB3;
    deviceSet(TUF_KB2, setting, sizeof(setting));
    setting[0] = 0xB4;
    result = deviceSet(TUF_KB2, setting, sizeof(setting));
  }

  return result == 1;
}

bool AtkAcpi::setTufKeyboardBrightness(int level) {
  if (!m_initialized && !initialize())
    return false;

  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;

  int param = 0x80 | (level & 0x7F);
  return deviceSet(TUF_KB_BRIGHTNESS, param) == 1;
}

int AtkAcpi::getTufKeyboardBrightness() {
  if (!m_initialized && !initialize())
    return -1;

  int result = deviceGet(TUF_KB_BRIGHTNESS);
  if (result < 0)
    return -1;

  return result & 0x03;
}

TufKeyboardState AtkAcpi::getTufKeyboardState() {
  TufKeyboardState state;
  state.isValid = false;
  state.brightness = 3;
  state.mode = TUF_MODE_STATIC;
  state.color = {255, 0, 0};
  state.speed = 1;

  if (!m_initialized && !initialize())
    return state;

  int brightness = getTufKeyboardBrightness();
  if (brightness >= 0) {
    state.brightness = brightness;
    state.isValid = true;
  }

  // Read Primary Device (TUF_KB)
  int kbControl = deviceGet(TUF_KB);
  int modeVal = 0;

  if (kbControl >= 0) {
    state.isValid = true;
    modeVal = kbControl & 0x0F;
  }

  // Read Secondary Device (TUF_KB2) - potentially more accurate for active
  // modes
  int kbControl2 = deviceGet(TUF_KB2);

  // LOGIC: If Primary is Static (0) but Secondary has a valid active mode,
  // trust Secondary.
  if (modeVal == 0 && kbControl2 >= 0) {
    int modeVal2 = kbControl2 & 0x0F;
    // If TUF_KB2 reports a known active mode (1=Breathing, 2=Cycle, 3=Strobe),
    // use it.
    if (modeVal2 > 0 && modeVal2 <= 3) {
      modeVal = modeVal2;
    }
  }

  if (modeVal <= 3) {
    state.mode = static_cast<TufRgbMode>(modeVal);
  }

  return state;
}
