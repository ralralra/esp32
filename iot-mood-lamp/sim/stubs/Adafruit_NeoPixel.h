// 네오픽셀 흉내 (시뮬레이터 전용) — 마지막으로 show()한 색과 밝기를 기억한다
#pragma once
#include "Arduino.h"

#define NEO_GRB    0x52
#define NEO_GRBW   0xD2
#define NEO_KHZ800 0x0000

struct PixelSim { uint8_t r = 0, g = 0, b = 0, w = 0; };

class Adafruit_NeoPixel {
 public:
  int count, pin, type;
  std::vector<PixelSim> buf, shown;
  uint8_t brightness = 255, shownBrightness = 0;
  int showCalls = 0;

  Adafruit_NeoPixel(int n, int p, int t) : count(n), pin(p), type(t), buf(n), shown(n) {}
  void begin() { pinMode(pin, OUTPUT); }
  void clear() { for (auto& px : buf) px = PixelSim(); }
  void setPixelColor(int i, uint8_t r, uint8_t g, uint8_t b) { setPixelColor(i, r, g, b, 0); }
  void setPixelColor(int i, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (i < 0 || i >= count) return;
    if (type != NEO_GRBW && w) sim::violation("RGB 링에 W 채널 값을 씀");
    buf[i] = {r, g, b, w};
  }
  void setBrightness(uint8_t b) { brightness = b; }
  void show() { shown = buf; shownBrightness = brightness; showCalls++; }

  // 시뮬레이터 확인용: 실제로 LED에 나가는 값(밝기 반영)의 합 → 전류 추정에 사용
  double shownDuty() const {
    double sum = 0;
    for (auto& px : shown) sum += (px.r + px.g + px.b + px.w) / 255.0;
    return sum * shownBrightness / 255.0;  // '채널 최대치'가 몇 개분 켜져 있는가
  }
};
