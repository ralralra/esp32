// 시뮬레이터용 ESP32Servo 흉내 — 각도와 attach 상태를 기록만 합니다.
#pragma once
#include "Arduino.h"

namespace sim {
  struct ServoState { bool attached = false; int angle = -1; int pin = -1; int moves = 0; };
  extern ServoState servoState[40];      // 핀 번호로 찾습니다
  extern bool allocatedTimer[4];
  // ESP32Servo 일부 버전은 detach해도 PWM 채널을 반납하지 않습니다.
  // 그 동작을 그대로 흉내내서, 붙였다 뗐다 하면 채널이 말라 attach가 실패하게 합니다.
  extern int  channelsUsed;
  constexpr int CHANNEL_LIMIT = 4;
}

class ESP32PWM {
 public:
  static void allocateTimer(int n) { if (n >= 0 && n < 4) sim::allocatedTimer[n] = true; }
};

class Servo {
 public:
  int attach(int pin, int, int) {
    // 실제 라이브러리는 쓸 타이머가 없으면 0을 돌려주며 실패합니다.
    bool any = false;
    for (bool t : sim::allocatedTimer) any = any || t;
    if (!any) return 0;                  // allocateTimer를 안 부른 경우 재현
    if (sim::channelsUsed >= sim::CHANNEL_LIMIT) return 0;   // 채널 고갈 재현
    sim::channelsUsed++;                 // ★ detach해도 돌려주지 않습니다
    pin_ = pin;
    sim::servoState[pin].attached = true;
    sim::servoState[pin].pin = pin;
    return 1;
  }
  bool attached() const { return pin_ >= 0 && sim::servoState[pin_].attached; }
  void detach() { if (pin_ >= 0) sim::servoState[pin_].attached = false; }
  void write(int angle) {
    if (pin_ < 0 || !sim::servoState[pin_].attached) return;   // 실패 시 아무 일도 없음
    if (sim::servoState[pin_].angle != angle) sim::servoState[pin_].moves++;
    sim::servoState[pin_].angle = angle;
  }
  void setPeriodHertz(int) {}
 private:
  int pin_ = -1;
};
