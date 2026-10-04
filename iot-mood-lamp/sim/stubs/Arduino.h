// PC에서 무드등 스케치를 돌려보기 위한 아두이노 흉내 헤더 (시뮬레이터 전용)
// 보드에 올리는 코드는 이 파일을 쓰지 않습니다.
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <cstdlib>

#define LOW    0
#define HIGH   1
#define INPUT  0
#define OUTPUT 1
#define INPUT_PULLUP 2

// ── 가상 보드 상태 ─────────────────────────────────────────
namespace sim {
  extern uint32_t nowMs;
  extern int  mode[40];         // -1 = 설정 안 함, INPUT, OUTPUT
  extern int  outLevel[40];     // digitalWrite로 써 둔 값 (출력 래치)
  extern int  inLevel[40];      // digitalRead가 돌려줄 값 (센서 흉내)
  extern int  analogIn[40];     // analogRead가 돌려줄 값 (0~4095)
  extern int  levelWhenOutput[40];  // pinMode(OUTPUT) 순간의 출력값 (부팅 깜빡임 검사)
  extern std::vector<std::string> serialOut;
  extern std::deque<char> serialIn;
  extern std::vector<std::string> violations;  // 전기 규칙 위반 기록
  void violation(const char* fmt, ...);

  inline bool inputOnly(int p) { return p == 34 || p == 35 || p == 36 || p == 39; }
  inline bool flashPin(int p)  { return p >= 6 && p <= 11; }
  inline bool adc1(int p)      { return p == 32 || p == 33 || (p >= 34 && p <= 39); }
}

inline uint32_t millis() { return sim::nowMs; }
inline void delay(uint32_t ms) { sim::nowMs += ms; }

inline void pinMode(uint8_t p, uint8_t m) {
  if (sim::flashPin(p)) sim::violation("GPIO%d: 내부 플래시 전용 핀 사용", p);
  if (m == OUTPUT && sim::inputOnly(p)) sim::violation("GPIO%d: 입력 전용 핀을 출력으로 설정", p);
  if (m == OUTPUT) sim::levelWhenOutput[p] = sim::outLevel[p];
  sim::mode[p] = m;
}
inline void digitalWrite(uint8_t p, uint8_t v) {
  if (sim::inputOnly(p)) sim::violation("GPIO%d: 입력 전용 핀에 digitalWrite", p);
  sim::outLevel[p] = v;
}
inline int digitalRead(uint8_t p) {
  return sim::mode[p] == OUTPUT ? sim::outLevel[p] : sim::inLevel[p];
}
inline int analogRead(uint8_t p) {
  if (!sim::adc1(p)) sim::violation("GPIO%d: ADC2 핀 analogRead — 와이파이를 켜면 읽기 불가", p);
  return sim::analogIn[p];
}

template <class T, class L, class H>
inline T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }

// ── 아두이노 String 흉내 ────────────────────────────────────
class String {
 public:
  std::string s;
  String() {}
  String(const char* v) : s(v ? v : "") {}
  String(const std::string& v) : s(v) {}
  String(char c) : s(1, c) {}
  String(int v) { s = std::to_string(v); }
  String(unsigned v) { s = std::to_string(v); }
  String(long v) { s = std::to_string(v); }
  String(unsigned long v) { s = std::to_string(v); }

  unsigned length() const { return static_cast<unsigned>(s.size()); }
  const char* c_str() const { return s.c_str(); }
  void trim() {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
  }
  void toLowerCase() { for (auto& c : s) c = static_cast<char>(std::tolower((unsigned char)c)); }
  void toUpperCase() { for (auto& c : s) c = static_cast<char>(std::toupper((unsigned char)c)); }
  char operator[](unsigned i) const { return i < s.size() ? s[i] : 0; }
  int indexOf(char c, int from = 0) const { size_t p = s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String& v, int from = 0) const { size_t p = s.find(v.s, from); return p == std::string::npos ? -1 : (int)p; }
  String substring(int a, int b) const { if (a >= (int)s.size()) return String(""); return String(s.substr(a, b - a)); }
  String substring(int a) const { return a >= (int)s.size() ? String("") : String(s.substr(a)); }
  bool startsWith(const char* v) const { return s.rfind(v, 0) == 0; }
  long toInt() const { try { return std::stol(s); } catch (...) { return 0; } }

  String& operator+=(char c) { s += c; return *this; }
  String& operator+=(const String& o) { s += o.s; return *this; }
  bool operator==(const char* o) const { return s == o; }
  bool operator==(const String& o) const { return s == o.s; }
};
inline String operator+(const String& a, const String& b) { return String(a.s + b.s); }
inline String operator+(const String& a, const char* b) { return String(a.s + b); }
inline String operator+(const char* a, const String& b) { return String(std::string(a) + b.s); }
inline String operator+(const String& a, int b) { return String(a.s + std::to_string(b)); }
inline String operator+(const String& a, unsigned long b) { return String(a.s + std::to_string(b)); }

// ── FreeRTOS 흉내 (ESP32의 Arduino.h는 FreeRTOS를 함께 불러온다) ──────────
#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) (ms)
typedef int BaseType_t;
struct QueueSim { size_t item; size_t cap; std::deque<std::string> q; };
typedef QueueSim* QueueHandle_t;
inline QueueHandle_t xQueueCreate(size_t n, size_t item) { return new QueueSim{item, n, {}}; }
inline BaseType_t xQueueSend(QueueHandle_t h, const void* p, int) {
  if (h->q.size() >= h->cap) return pdFALSE;
  h->q.emplace_back((const char*)p, h->item); return pdTRUE;
}
inline BaseType_t xQueueReceive(QueueHandle_t h, void* p, int) {
  if (h->q.empty()) return pdFALSE;
  std::memcpy(p, h->q.front().data(), h->item); h->q.pop_front(); return pdTRUE;
}
inline int uxQueueSpacesAvailable(QueueHandle_t h) { return (int)(h->cap - h->q.size()); }
inline BaseType_t xTaskCreatePinnedToCore(void (*)(void*), const char*, int, void*, int, void*, int) { return pdTRUE; }
inline void vTaskDelay(int) {}
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))

// ── Serial 흉내 ─────────────────────────────────────────────
class SerialSim {
 public:
  void begin(unsigned long) {}
  void println(const char* v) { sim::serialOut.push_back(v); }
  void println(const String& v) { sim::serialOut.push_back(v.s); }
  void printf(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    sim::serialOut.push_back(line);
  }
  int available() { return static_cast<int>(sim::serialIn.size()); }
  int read() {
    if (sim::serialIn.empty()) return -1;
    char c = sim::serialIn.front(); sim::serialIn.pop_front(); return c;
  }
};
extern SerialSim Serial;
