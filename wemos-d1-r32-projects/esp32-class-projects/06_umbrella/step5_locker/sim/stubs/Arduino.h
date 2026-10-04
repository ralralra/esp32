// PC에서 스케치를 돌려보기 위한 아두이노 흉내 헤더 (시뮬레이터 전용)
// 보드에 올리는 코드는 이 파일을 쓰지 않습니다.
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

#define LOW  0
#define HIGH 1
#define INPUT_PULLUP 2

// ── 가상 시계 ──────────────────────────────────────────────
namespace sim {
  extern uint32_t nowMs;
  extern int      pinLevel[40];       // digitalRead가 돌려줄 값
  extern std::vector<std::string> serialOut;
  extern std::string serialIn;        // 시리얼로 넣을 명령
}

inline uint32_t millis() { return sim::nowMs; }
inline void delay(uint32_t ms) { sim::nowMs += ms; }

inline void pinMode(uint8_t, uint8_t) {}
inline int  digitalRead(uint8_t pin) { return sim::pinLevel[pin]; }

inline bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

// ── 아두이노 String 흉내 ────────────────────────────────────
class String {
 public:
  std::string s;
  String() {}
  String(const char* v) : s(v ? v : "") {}
  String(const std::string& v) : s(v) {}
  String(char c) : s(1, c) {}
  String(int v)               { s = std::to_string(v); }
  String(unsigned int v)      { s = std::to_string(v); }
  String(long v)              { s = std::to_string(v); }
  String(unsigned long v)     { s = std::to_string(v); }

  unsigned length() const { return static_cast<unsigned>(s.size()); }
  const char* c_str() const { return s.c_str(); }
  char operator[](int i) const { return s[static_cast<size_t>(i)]; }

  void trim() {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
  }
  void toUpperCase() { for (auto& c : s) c = static_cast<char>(std::toupper((unsigned char)c)); }
  void replace(char from, char to) { for (auto& c : s) if (c == from) c = to; }
  void replace(const char* from, const char* to) {
    std::string f(from), t(to);
    if (f.empty()) return;
    size_t p = 0;
    while ((p = s.find(f, p)) != std::string::npos) { s.replace(p, f.size(), t); p += t.size(); }
  }
  int indexOf(char c, int from = 0) const {
    size_t p = s.find(c, static_cast<size_t>(from));
    return p == std::string::npos ? -1 : static_cast<int>(p);
  }
  int indexOf(const char* v, int from = 0) const {
    size_t p = s.find(v, static_cast<size_t>(from));
    return p == std::string::npos ? -1 : static_cast<int>(p);
  }
  String substring(int a) const { return String(s.substr(static_cast<size_t>(a))); }
  String substring(int a, int b) const {
    return String(s.substr(static_cast<size_t>(a), static_cast<size_t>(b - a)));
  }
  bool startsWith(const char* v) const { return s.rfind(v, 0) == 0; }
  bool equalsIgnoreCase(const char* v) const {
    std::string o(v);
    if (o.size() != s.size()) return false;
    for (size_t i = 0; i < s.size(); ++i)
      if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)o[i])) return false;
    return true;
  }
  long toInt() const { try { return std::stol(s); } catch (...) { return 0; } }
  void toCharArray(char* buf, unsigned size) const {
    std::snprintf(buf, size, "%s", s.c_str());
  }

  String& operator+=(const String& o) { s += o.s; return *this; }
  String& operator+=(const char* o)   { s += o;   return *this; }
  bool operator==(const char* o) const { return s == o; }
  bool operator==(const String& o) const { return s == o.s; }
  bool operator!=(const char* o) const { return s != o; }
};

inline String operator+(const String& a, const String& b) { return String(a.s + b.s); }
inline String operator+(const String& a, const char* b)   { return String(a.s + b); }
inline String operator+(const char* a, const String& b)   { return String(std::string(a) + b.s); }

// ── Serial 흉내 ─────────────────────────────────────────────
class SerialSim {
 public:
  void begin(unsigned long) {}
  void println() { sim::serialOut.push_back(""); }
  void println(const char* v) { sim::serialOut.push_back(v); }
  void println(const String& v) { sim::serialOut.push_back(v.s); }
  void print(const char* v) { sim::serialOut.push_back(v); }
  void printf(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    sim::serialOut.push_back(line);
  }
  int available() { return sim::serialIn.empty() ? 0 : 1; }
  String readStringUntil(char) {
    std::string out = sim::serialIn;
    sim::serialIn.clear();
    return String(out);
  }
};
extern SerialSim Serial;
