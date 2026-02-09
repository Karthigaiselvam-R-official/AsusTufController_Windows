#ifndef AURAHID_H
#define AURAHID_H

#include <string>
#include <vector>

// Forward declare Windows types to avoid header conflicts
typedef void *AuraDeviceHandle;

// ASUS USB Vendor ID
#define ASUS_USB_VENDOR_ID 0x0B05

// RGB Modes (from rogauracore protocol)
enum AuraMode {
  AURA_MODE_STATIC = 0,
  AURA_MODE_BREATHING = 1,
  AURA_MODE_COLOR_CYCLE = 2, // Rainbow
  AURA_MODE_STROBE = 3
};

// Speed values
enum AuraSpeed {
  AURA_SPEED_SLOW = 0xE1,
  AURA_SPEED_MEDIUM = 0xEB,
  AURA_SPEED_FAST = 0xF5
};

struct AuraColor {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

class AuraHID {
public:
  static AuraHID &instance();

  bool initialize();
  void shutdown();
  bool isConnected() const;

  // Control functions
  bool setMode(AuraMode mode, const AuraColor &color,
               AuraSpeed speed = AURA_SPEED_MEDIUM);
  bool setBrightness(int level); // 0-3
  bool initializeKeyboard();
  bool apply();

  // Get connected device info
  unsigned short getProductId() const;

private:
  AuraHID();
  ~AuraHID();
  AuraHID(const AuraHID &) = delete;
  AuraHID &operator=(const AuraHID &) = delete;

  bool findAsusKeyboard();
  bool sendFeatureReport(const unsigned char *data, size_t length);
  void closeDevice();

  AuraDeviceHandle m_deviceHandle;
  unsigned short m_productId;
  bool m_initialized;
};

#endif // AURAHID_H
