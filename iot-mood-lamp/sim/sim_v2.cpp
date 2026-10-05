// V2 네오픽셀 컬러형 — step2_auto_mood_lamp.ino 를 PC에서 시나리오대로 돌려본다
#include "sim_common.h"
#include "../v2_neopixel_color/firmware/step2_auto_mood_lamp/step2_auto_mood_lamp.ino"

static const int PIR = 25, LIGHT = 34;
static const double MA_PER_CHANNEL = 20.0;   // LED 칩 한 색(채널) 최대 약 20mA 기준
template <class F> static void run(uint32_t ms, F onTick) {
  for (uint32_t t = 0; t < ms; t += 10) { onTick(); loop(); sim::nowMs += 10; }
}
static void run(uint32_t ms) { run(ms, [] {}); }
static int lit() { return ring.shownBrightness; }
static double ringmA() { return ring.shownDuty() * MA_PER_CHANNEL; }
static bool firstPixelIs(int r, int g, int b, int w) {
  auto& p = ring.shown[0]; return p.r == r && p.g == g && p.b == b && p.w == w;
}

int main() {
  std::printf("════ IoT 무드등 V2 (네오픽셀 컬러형) 시뮬레이션 ════\n");
  std::printf("배선: 네오픽셀 GPIO%d (%d구 %s) · PIR GPIO%d · 조도 GPIO%d\n",
              LED_PIN, LED_COUNT, LED_RGBW ? "RGBW" : "RGB", PIR, LIGHT);
  sim::reset();
  sim::analogIn[LIGHT] = 3000;
  setup();

  section("1. 부팅 직후");
  check(sim::mode[LED_PIN] == OUTPUT, "네오픽셀 데이터 핀이 출력으로 설정됨");
  check(ring.showCalls >= 1 && ring.shownDuty() == 0, "모든 LED 꺼진 상태로 시작");

  section("2. 어두움 + 사람 감지 → 웜화이트로 서서히 켜짐");
  sim::analogIn[LIGHT] = 800; run(3000);
  sim::inLevel[PIR] = HIGH;
  run(500);
  int mid = lit();
  run(700);
  int target = MAX_BRIGHTNESS * 60 / 100;
  std::printf("     밝기 출력: 0.5초 %d → 1.2초 %d (목표 %d)\n", mid, lit(), target);
  check(mid > 0 && mid < target, "0.5초 시점: 켜지는 중 (페이드)");
  check(lit() == target, "1.2초 시점: 목표 밝기(60%) 도달");
  check(firstPixelIs(0, 0, 0, 255), "색 = 웜화이트 (W 채널)");

  section("3. 색 바꾸기");
  sendLine("c red"); run(50);
  check(firstPixelIs(255, 0, 0, 0), "'c red' → 빨강");
  sendLine("c 10 200 30"); run(50);
  check(firstPixelIs(10, 200, 30, 0), "'c 10 200 30' → 사용자 색");
  sendLine("c rainbow"); run(50);
  check(firstPixelIs(10, 200, 30, 0), "없는 색 이름은 무시 (이전 색 유지)");

  section("4. 밝기 바꾸기와 상한");
  sendLine("b 150"); run(1200);
  check(brightPct == 100 && lit() == MAX_BRIGHTNESS, "'b 150' → 100%로 제한, 출력은 상한(MAX_BRIGHTNESS)");
  sendLine("c white"); run(1200);
  std::printf("     최악 조건(흰색+W 100%%, 상한 %d/255) 링 전류 ≈ %.0f mA\n", MAX_BRIGHTNESS, ringmA());
  check(ringmA() < 1500, "링 전류 1.5A 미만 — 5V 2A 어댑터 안에서 ESP32 몫 여유");
  sendLine("b 30"); run(1200);
  check(lit() == MAX_BRIGHTNESS * 30 / 100, "'b 30' → 30%로 서서히 내려감");

  section("5. 사람이 나감 — 5분 뒤 서서히 꺼짐");
  sim::inLevel[PIR] = LOW;
  run(4 * 60 * 1000 + 50 * 1000);
  check(lampOn && lit() > 0, "4분 50초 시점: 아직 켜짐");
  run(20 * 1000);
  check(!lampOn && lit() == 0, "5분 10초 시점: 완전히 꺼짐");

  section("6. 'sensor' — 센서 값 확인 출력");
  size_t m0 = sim::serialOut.size();
  sendLine("sensor"); run(50);
  check(sim::serialOut.size() > m0 && sim::serialOut.back().find("[센서] 조도=") != std::string::npos, "'sensor' 입력 즉시 센서 한 줄 출력");
  size_t m1 = sim::serialOut.size();
  run(3000);
  int lines = 0;
  for (size_t i = m1; i < sim::serialOut.size(); i++) if (sim::serialOut[i].rfind("[센서] 조도=", 0) == 0) lines++;
  check(lines == 3, "3초 동안 1초마다 3줄");
  sim::analogIn[LIGHT] = 500; run(1100);
  check(sim::serialOut.back().find("어두움") != std::string::npos && sim::serialOut.back().find("기준: 켜짐≤1200 꺼짐≥1500") != std::string::npos, "조도 500 → '어두움' + 기준값 표시");
  size_t m2 = sim::serialOut.size();
  sim::inLevel[PIR] = HIGH; run(20);
  bool edge = false;
  for (size_t i = m2; i < sim::serialOut.size(); i++) if (sim::serialOut[i].find("PIR 감지됨") != std::string::npos) edge = true;
  check(edge, "PIR가 바뀌는 순간 바로 '감지됨' 알림");
  sim::inLevel[PIR] = LOW; run(20);
  sendLine("sensor"); run(50);
  size_t m3 = sim::serialOut.size();
  run(3000);
  check(sim::serialOut.size() == m3, "'sensor' 다시 입력 → 출력 멈춤");
  printSerialSince(m0 > 2 ? m0 : 0);
  sim::analogIn[LIGHT] = 3000; run(3000);

  section("7. 수동 모드");
  sim::analogIn[LIGHT] = 3000;
  size_t mark = sim::serialOut.size();
  sendLine("on"); run(1200);
  check(lampOn && lit() == MAX_BRIGHTNESS * 30 / 100, "'on' → 마지막 색·밝기(30%)로 켜짐");
  sendLine("off"); run(1200);
  check(lit() == 0, "'off' → 꺼짐");
  sendLine("auto"); sendLine("s"); run(100);
  check(autoMode, "'auto' → 자동 모드 복귀");
  printSerialSince(mark);

  return finish("V2");
}
