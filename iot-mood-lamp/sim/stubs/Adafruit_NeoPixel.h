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
  void setPixelColor(int i, uint32_t c) {
    if (i < 0 || i >= count) return;
    buf[i] = {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), uint8_t(c >> 24)};
  }
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return (uint32_t(r) << 16) | (uint32_t(g) << 8) | b; }
  static uint32_t ColorHSV(uint16_t hue) {          // 색상환 6구간 근사 (실물 라이브러리와 색 순서 같음)
    uint32_t h = (uint32_t(hue) * 1530 + 32768) / 65536, r, g, b;
    if (h < 255) { r = 255; g = h; b = 0; } else if (h < 510) { r = 510 - h; g = 255; b = 0; }
    else if (h < 765) { r = 0; g = 255; b = h - 510; } else if (h < 1020) { r = 0; g = 1020 - h; b = 255; }
    else if (h < 1275) { r = h - 1020; g = 0; b = 255; } else { r = 255; g = 0; b = h < 1530 ? 1530 - h : 0; }
    return Color(r, g, b);
  }
  static uint32_t gamma32(uint32_t c) { return c; }
  void setBrightness(uint8_t b) { brightness = b; }
  void show() { shown = buf; shownBrightness = brightness; showCalls++; }

  // 시뮬레이터 확인용: 실제로 LED에 나가는 값(밝기 반영)의 합 → 전류 추정에 사용
  double shownDuty() const {
    double sum = 0;
    for (auto& px : shown) sum += (px.r + px.g + px.b + px.w) / 255.0;
    return sum * shownBrightness / 255.0;  // '채널 최대치'가 몇 개분 켜져 있는가
  }
};
