// 와이파이 흉내 (시뮬레이터 전용) — sim::wifiUp 으로 연결 상태를 바꾼다
#pragma once
#include "Arduino.h"
#define WIFI_STA 1
#define WL_CONNECTED 3
namespace sim { extern bool wifiUp; extern int wifiBegins; }
class WiFiSim {
 public:
  void mode(int) {}
  void begin(const char*, const char*) { sim::wifiBegins++; }
  void disconnect() {}
  int status() { return sim::wifiUp ? WL_CONNECTED : 0; }
};
extern WiFiSim WiFi;
