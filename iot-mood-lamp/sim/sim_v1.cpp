// V1 릴레이 전구형 — step3_auto_lamp.ino 를 PC에서 시나리오대로 돌려본다
#include "sim_common.h"
#include "../v1_relay_bulb/firmware/step3_auto_lamp/step3_auto_lamp.ino"

static const int RELAY = 26, PIR = 25, LIGHT = 34;
static bool bulbOn() { return digitalRead(RELAY) == (RELAY_ACTIVE_LOW ? LOW : HIGH); }
// 10ms마다 loop()를 부르며 시간을 흘린다 (onTick: 매 틱마다 센서를 바꾸는 함수)
template <class F> static void run(uint32_t ms, F onTick) {
  for (uint32_t t = 0; t < ms; t += 10) { onTick(); loop(); sim::nowMs += 10; }
}
static void run(uint32_t ms) { run(ms, [] {}); }

int main() {
  std::printf("════ IoT 무드등 V1 (릴레이 전구형) 시뮬레이션 ════\n");
  std::printf("배선: 릴레이 GPIO%d · PIR GPIO%d · 조도 GPIO%d  |  조도값: 밝음 3000 / 어두움 800\n", RELAY, PIR, LIGHT);
  sim::reset();
  sim::analogIn[LIGHT] = 3000;          // 밝은 교실
  setup();

  section("1. 부팅 직후");
  check(sim::levelWhenOutput[RELAY] == (RELAY_ACTIVE_LOW ? HIGH : LOW), "출력으로 바꾸는 순간 릴레이 핀이 '꺼짐' 값 (부팅 깜빡임 없음)");
  check(!bulbOn(), "전구 꺼짐");

  section("2. 밝을 때 사람이 지나감");
  sim::inLevel[PIR] = HIGH; run(2000); sim::inLevel[PIR] = LOW; run(1000);
  check(!bulbOn(), "밝으면 사람이 있어도 켜지지 않음");

  section("3. 경계값 근처(1300~1400)에서 흔들림 — 히스테리시스");
  run(3000, [] { sim::analogIn[LIGHT] = (sim::nowMs / 200) % 2 ? 1300 : 1400; });
  sim::inLevel[PIR] = HIGH; run(500); sim::inLevel[PIR] = LOW;
  check(!bulbOn(), "어두움 기준(1200) 아래로 내려가지 않으면 켜지지 않음");

  section("4. 불을 끄고(어두움) 사람이 들어옴");
  sim::analogIn[LIGHT] = 800; run(3000);
  check(isDark, "조도 800 → '어두움' 판정");
  run(3000, [] { sim::analogIn[LIGHT] = (sim::nowMs / 200) % 2 ? 1300 : 1400; });
  check(isDark, "어두워진 뒤 1300~1400으로 흔들려도 '어두움' 유지 (밝음 기준 1500 미만)");
  sim::analogIn[LIGHT] = 800; run(1000);
  size_t mark = sim::serialOut.size();
  uint32_t t0 = sim::nowMs;
  sim::inLevel[PIR] = HIGH;
  while (!bulbOn() && sim::nowMs - t0 < 2000) run(10);
  check(bulbOn(), "전구 켜짐");
  check(sim::nowMs - t0 <= 50, "감지 후 50ms 안에 반응");
  printSerialSince(mark);

  section("5. 전구 빛이 조도센서에 들어옴 (조도 3500)");
  sim::analogIn[LIGHT] = 3500;
  run(4 * 60 * 1000, [] { sim::inLevel[PIR] = (sim::nowMs / 30000) % 2 ? LOW : HIGH; });  // 30초마다 움직임
  check(bulbOn(), "켜진 동안에는 밝아져도 꺼지지 않음 (자기 빛 무시)");

  section("6. 사람이 나감 — 5분 뒤 자동 끄기");
  sim::analogIn[LIGHT] = 800;
  sim::inLevel[PIR] = LOW;
  uint32_t leave = sim::nowMs;
  mark = sim::serialOut.size();
  run(4 * 60 * 1000 + 50 * 1000);
  check(bulbOn(), "4분 50초 시점: 아직 켜짐");
  run(20 * 1000);
  check(!bulbOn(), "5분 10초 시점: 꺼짐");
  std::printf("     (마지막 감지 %s → 꺼짐 확인 %s)\n", fmtTime(leave).c_str(), fmtTime(sim::nowMs).c_str());
  printSerialSince(mark);

  section("7. 수동 모드 (시리얼 명령)");
  sim::analogIn[LIGHT] = 3000;
  mark = sim::serialOut.size();
  sendLine("on"); run(100);
  check(bulbOn(), "'on' → 밝은데도 켜짐");
  run(10 * 60 * 1000);
  check(bulbOn(), "사람이 없어도 10분 동안 계속 켜짐 (수동)");
  sendLine("off"); run(100);
  check(!bulbOn(), "'off' → 꺼짐");
  sendLine("auto"); sendLine("s"); run(100);
  check(autoMode, "'auto' → 자동 모드 복귀");
  printSerialSince(mark);

  return finish("V1");
}
