// V1 · 4단계 앱 연동 펌웨어 — 가짜 서버와 함께 시나리오대로 돌려본다
#include "sim_server.h"
#include "../v1_relay_bulb/firmware/step4_app_lamp/step4_app_lamp.ino"

static const int RELAY = 26, PIR = 25, LIGHT = 34;
static bool bulbOn() { return digitalRead(RELAY) == (RELAY_ACTIVE_LOW ? LOW : HIGH); }
static int raw(int pct) { return pct * 4095 / 100; }
template <class F> static void run(uint32_t ms, F onTick) {
  for (uint32_t t = 0; t < ms; t += 10) { onTick(); loop(); networkStep(sim::nowMs); sim::nowMs += 10; }
}
static void run(uint32_t ms) { run(ms, [] {}); }
static std::string rep(const char* k) { return sim::state["TEAM01"][k]; }
template <class P> static uint32_t until(P pred, uint32_t maxMs) {
  uint32_t t0 = sim::nowMs; while (!pred() && sim::nowMs - t0 < maxMs) run(10); return sim::nowMs - t0;
}

int main() {
  std::printf("════ IoT 무드등 V1 · 앱 연동 펌웨어 시뮬레이션 (TEAM01) ════\n");
  sim::reset();
  sim::analogIn[LIGHT] = raw(80);
  setup();

  section("1. 부팅 → 와이파이 연결 → 첫 보고");
  run(1000);
  check(!bulbOn() && sim::calls["report"] == 0, "와이파이 연결 전: 꺼짐, 서버 요청 없음");
  sim::wifiUp = true;
  run(500);
  check(rep("team") == "TEAM01" && rep("type") == "bulb" && rep("power") == "OFF", "연결 즉시 보고: team=TEAM01 · type=bulb · power=OFF");
  check(rep("dark") == "30" && rep("off") == "300", "기본 설정 보고: 어두움 30% · 자동 꺼짐 300초");

  section("2. 앱에서 '켜기'");
  sim::appSet("TEAM01", "ON");
  uint32_t ms = until([] { return bulbOn(); }, 5000);
  std::printf("     앱 명령 → 전구 켜짐까지 %u ms (폴링 간격 %lu ms)\n", ms, POLL_MS);
  check(bulbOn() && ms <= POLL_MS + 50, "폴링 한 번 안에 켜짐");
  run(200);
  check(rep("power") == "ON" && rep("auto") == "0", "바로 보고: power=ON · auto=0(수동)");

  section("3. 다른 팀 명령은 무시");
  sim::appSet("TEAM02", "OFF");
  run(5000);
  check(bulbOn() && sim::queues["TEAM02"].size() == 1, "TEAM02의 OFF는 TEAM01 전구에 영향 없음");

  section("4. V1에 색·밝기 명령");
  sim::appSet("TEAM01", "COLOR", "blue");
  run(2500);
  check(bulbOn() && sim::queues["TEAM01"].empty(), "COLOR는 꺼내서 무시 — 전구 그대로");

  section("5. 자동 설정(CONFIG) → 자동 동작");
  sim::appSet("TEAM01", "CONFIG", "", 40, 30);
  run(2500);
  check(darkPct == 40 && autoOffMs == 30000, "어두움 기준 40% · 자동 꺼짐 30초 적용");
  check(sim::nvs["dark"] == 40 && sim::nvs["off"] == 30, "보드 저장공간에 저장");
  run(200);
  check(rep("dark") == "40" && rep("off") == "30", "앱에 보이도록 보고");
  sim::appSet("TEAM01", "OFF"); run(2500);
  sim::appSet("TEAM01", "AUTO"); run(2500);
  check(autoMode && !bulbOn(), "AUTO → 자동 모드, 꺼진 상태에서 시작");
  sim::analogIn[LIGHT] = raw(35);
  run(3000);
  check(isDark, "주변 밝기 35% ≤ 기준 40% → 어두움");
  sim::inLevel[PIR] = HIGH;
  run(100);
  check(bulbOn(), "사람 감지 → 자동 켜짐");
  sim::inLevel[PIR] = LOW;
  run(25000);
  check(bulbOn(), "25초 뒤: 아직 켜짐 (설정 30초)");
  run(6000);
  check(!bulbOn(), "31초 뒤: 자동 꺼짐");
  run(300);
  check(rep("power") == "OFF", "자동 꺼짐도 앱에 보고");

  section("6. 와이파이가 끊겨도 자동 동작은 계속");
  sim::wifiUp = false;
  run(500);
  sim::appSet("TEAM01", "ON");                 // 끊긴 동안 앱이 보낸 명령은 서버에 대기
  sim::inLevel[PIR] = HIGH; run(100); sim::inLevel[PIR] = LOW;
  check(bulbOn(), "끊긴 상태에서도 어두움 + 사람 → 켜짐");
  int before = sim::calls["next"];
  run(5000);
  check(sim::calls["next"] == before, "끊긴 동안 서버 요청 없음");
  sim::wifiUp = true;
  until([] { return !autoMode; }, 5000);
  check(!autoMode && bulbOn(), "다시 연결 → 밀린 '켜기' 받아서 수동 모드");

  section("7. 서버 오류 — 재시도 홍수 없음");
  sim::serverDown = true;
  int f0 = sim::calls["fail"];
  run(20000);
  int fails = sim::calls["fail"] - f0;
  std::printf("     서버가 죽은 20초 동안 시도 %d번\n", fails);
  check(fails <= 5, "실패 후 5초씩 쉬고 다시 (20초에 5번 이하)");
  sim::serverDown = false;
  sim::appSet("TEAM01", "OFF");
  until([] { return !bulbOn(); }, 8000);
  check(!bulbOn(), "서버가 돌아오면 다시 명령 처리");

  section("8. 바뀐 게 없어도 30초마다 보고");
  int r0 = sim::calls["report"];
  run(65000);
  check(sim::calls["report"] - r0 >= 2, "65초 동안 보고 2번 이상 (앱의 '연결됨' 유지)");

  section("9. 재부팅해도 설정 유지");
  darkPct = 0; autoOffMs = 0;
  setup();
  check(darkPct == 40 && autoOffMs == 30000, "다시 켜도 어두움 40% · 30초 그대로");

  return finish("V1 앱 연동");
}
