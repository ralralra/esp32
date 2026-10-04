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

  unsigned length() const { return static_cast<unsigned>(s.size()); }
  const char* c_str() const { return s.c_str(); }
  void trim() {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
  }
  void toLowerCase() { for (auto& c : s) c = static_cast<char>(std::tolower((unsigned char)c)); }
  String substring(int a) const { return a >= (int)s.size() ? String("") : String(s.substr(a)); }
  bool startsWith(const char* v) const { return s.rfind(v, 0) == 0; }
  long toInt() const { try { return std::stol(s); } catch (...) { return 0; } }

  String& operator+=(char c) { s += c; return *this; }
  String& operator+=(const String& o) { s += o.s; return *this; }
  bool operator==(const char* o) const { return s == o; }
  bool operator==(const String& o) const { return s == o.s; }
};

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
