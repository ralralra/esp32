// 시뮬레이터 공통 — 가상 보드 상태, 시간 진행, 검사 도구
#pragma once
#include "Arduino.h"
#include <cstdlib>

namespace sim {
  uint32_t nowMs = 0;
  int  mode[40], outLevel[40], inLevel[40], analogIn[40], levelWhenOutput[40];
  std::vector<std::string> serialOut;
  std::deque<char> serialIn;
  std::vector<std::string> violations;
  void violation(const char* fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt); std::vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (auto& v : violations) if (v == buf) return;
    violations.push_back(buf);
  }
  void reset() {
    nowMs = 0;
    for (int i = 0; i < 40; i++) { mode[i] = -1; outLevel[i] = LOW; inLevel[i] = LOW; analogIn[i] = 0; levelWhenOutput[i] = -1; }
    serialOut.clear(); serialIn.clear(); violations.clear();
  }
}
SerialSim Serial;

static int passCount = 0, failCount = 0;
static void check(bool ok, const char* what) {
  std::printf("  %s %s\n", ok ? "✅" : "❌", what);
  ok ? passCount++ : failCount++;
}
static void section(const char* title) { std::printf("\n▶ %s\n", title); }
static void sendLine(const char* s) { while (*s) sim::serialIn.push_back(*s++); sim::serialIn.push_back('\n'); }
[[maybe_unused]] static std::string fmtTime(uint32_t ms) {
  char b[32]; std::snprintf(b, sizeof b, "%u:%02u.%01u", ms / 60000, (ms / 1000) % 60, (ms % 1000) / 100); return b;
}
static void printSerialSince(size_t from) {
  for (size_t i = from; i < sim::serialOut.size(); i++) std::printf("     │ %s\n", sim::serialOut[i].c_str());
}
static int finish(const char* name) {
  std::printf("\n전기 규칙 위반: %s\n", sim::violations.empty() ? "없음" : "");
  for (auto& v : sim::violations) std::printf("  ⚠ %s\n", v.c_str());
  std::printf("\n%s 결과: 통과 %d / 실패 %d\n", name, passCount, failCount + (int)sim::violations.size());
  return (failCount || !sim::violations.empty()) ? 1 : 0;
}
