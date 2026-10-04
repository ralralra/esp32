#pragma once
#include "Arduino.h"
#define WIFI_STA 1
#define WL_CONNECTED 3
class WiFiSim {
 public:
  void mode(int) {}
  void setAutoReconnect(bool) {}
  void persistent(bool) {}
  void begin(const char*, const char*) {}
  void disconnect() {}
  int  status() { return 0; }
};
extern WiFiSim WiFi;
