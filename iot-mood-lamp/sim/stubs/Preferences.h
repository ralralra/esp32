// 보드 저장공간 흉내 (시뮬레이터 전용) — 재부팅 시험을 위해 sim::nvs 에 남는다
#pragma once
#include "Arduino.h"
namespace sim { extern std::map<std::string, int> nvs; }
class Preferences {
 public:
  bool begin(const char*, bool) { return true; }
  int getInt(const char* k, int def) { auto it = sim::nvs.find(k); return it == sim::nvs.end() ? def : it->second; }
  void putInt(const char* k, int v) { sim::nvs[k] = v; }
};
