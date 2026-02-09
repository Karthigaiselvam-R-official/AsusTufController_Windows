#ifndef ATKACPI_H
#define ATKACPI_H

typedef void *AtkDeviceHandle;

#define TUF_KB 0x00100056
#define TUF_KB2 0x0010005a
#define TUF_KB_BRIGHTNESS 0x00050021
#define TUF_KB_STATE 0x00100057
#define ATK_CONTROL_CODE 0x0022240C
#define ATK_DEVS 0x53564544
#define ATK_DSTS 0x53545344

enum TufRgbMode {
  TUF_MODE_STATIC = 0,
  TUF_MODE_BREATHING = 1,
  TUF_MODE_COLOR_CYCLE = 2,
  TUF_MODE_STROBE = 3,
  TUF_MODE_UNKNOWN = -1
};

enum TufRgbSpeed {
  TUF_SPEED_SLOW = 0xE1,
  TUF_SPEED_MEDIUM = 0xEB,
  TUF_SPEED_FAST = 0xF5
};

struct TufColor {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

struct TufKeyboardState {
  bool isValid;
  int brightness;
  TufRgbMode mode;
  TufColor color;
  int speed;
};

class AtkAcpi {
public:
  static AtkAcpi &instance();

  bool initialize();
  void shutdown();
  bool isConnected() const;

  int deviceSet(unsigned int deviceId, int value);
  int deviceSet(unsigned int deviceId, unsigned char *params, int paramLen);
  int deviceGet(unsigned int deviceId);

  bool setTufKeyboardRGB(TufRgbMode mode, const TufColor &color, int speed);
  bool setTufKeyboardBrightness(int level);
  int getTufKeyboardBrightness();
  TufKeyboardState getTufKeyboardState();

private:
  AtkAcpi();
  ~AtkAcpi();
  AtkAcpi(const AtkAcpi &) = delete;
  AtkAcpi &operator=(const AtkAcpi &) = delete;

  bool callMethod(unsigned int methodId, unsigned char *args, int argLen,
                  unsigned char *outBuffer, int outLen);

  AtkDeviceHandle m_handle;
  bool m_initialized;
};

#endif
