// V2 · 3단계 앱 연동 펌웨어 — 가짜 서버와 함께 시나리오대로 돌려본다
#include "sim_server.h"
#include "../v2_neopixel_color/firmware/step3_app_mood_lamp/step3_app_mood_lamp.ino"

static const int PIR = 25, LIGHT = 34;
static int raw(int pct) { return pct * 4095 / 100; }
template <class F> static void run(uint32_t ms, F onTick) {
  for (uint32_t t = 0; t < ms; t += 10) { onTick(); loop(); networkStep(sim::nowMs); sim::nowMs += 10; }
}
static void run(uint32_t ms) { run(ms, [] {}); }
static std::string rep(const char* k) { return sim::state["TEAM05"][k]; }
static bool px(int i, int r, int g, int b, int w) { auto& p = ring.shown[i]; return p.r == r && p.g == g && p.b == b && p.w == w; }

int main() {
  std::printf("════ IoT 무드등 V2 · 앱 연동 펌웨어 시뮬레이션 (TEAM05) ════\n");
  TEAM_ID = "TEAM05";
  sim::reset();
  sim::analogIn[LIGHT] = raw(80);
  setup();
  sim::wifiUp = true;
  run(500);

  section("1. 첫 보고");
  check(rep("type") == "neopixel" && rep("color") == "warm" && rep("bright") == "60", "type=neopixel · color=warm · bright=60");
  check(ring.shownDuty() == 0, "부팅 직후 LED 모두 꺼짐");

  section("2. 앱 '켜기' → 서서히 켜짐");
  sim::appSet("TEAM05", "ON");
  run(2000 + 500);
  int mid = ring.shownBrightness;
  run(1000);
  std::printf("     밝기 출력: 중간 %d → 끝 %d (목표 %d)\n", mid, ring.shownBrightness, MAX_BRIGHTNESS * 60 / 100);
  check(ring.shownBrightness == MAX_BRIGHTNESS * 60 / 100, "목표 밝기(60%) 도달");
  check(px(0, 0, 0, 0, 255), "웜화이트(W 채널)");

  section("3. 색 바꾸기 — 이름 · 색 코드 · 무지개");
  sim::appSet("TEAM05", "COLOR", "blue"); run(2100);
  check(px(0, 0, 60, 255, 0), "blue");
  sim::appSet("TEAM05", "COLOR", "FF8800"); run(2100);
  check(px(0, 255, 136, 0, 0) && rep("color") == "FF8800", "6자리 색 코드 FF8800 → (255,136,0) · 보고도 FF8800");
  sim::appSet("TEAM05", "COLOR", "rainbo"); run(2100);
  check(px(0, 255, 136, 0, 0), "철자가 틀린 색은 무시 (이전 색 유지)");
  sim::appSet("TEAM05", "COLOR", "rainbow"); run(2100);
  auto first = ring.shown[0];
  bool ringVaries = !(ring.shown[0].r == ring.shown[8].r && ring.shown[0].g == ring.shown[8].g && ring.shown[0].b == ring.shown[8].b);
  run(2000);
  bool moved = !(ring.shown[0].r == first.r && ring.shown[0].g == first.g && ring.shown[0].b == first.b);
  check(ringVaries, "무지개: 링 둘레마다 다른 색");
  check(moved && rep("color") == "rainbow", "무지개: 시간이 지나면 색이 돈다 · 보고 color=rainbow");

  section("4. 밝기 슬라이더 연타 → 마지막 값만");
  for (int v : {20, 45, 70, 80}) sim::appSet("TEAM05", "BRIGHT", std::to_string(v));
  run(2100 + 1100);
  check(brightPct == 80 && ring.shownBrightness == MAX_BRIGHTNESS * 80 / 100, "4번 보냈지만 80% 한 번만 적용");
  check(sim::queues["TEAM05"].empty(), "서버 대기열 비움");

  section("5. CONFIG + 자동 끄기 (서서히)");
  sim::appSet("TEAM05", "CONFIG", "", 50, 20); run(2100);
  sim::appSet("TEAM05", "AUTO"); run(2100);
  sim::analogIn[LIGHT] = raw(30); run(3000);
  sim::inLevel[PIR] = HIGH; run(100); sim::inLevel[PIR] = LOW;
  run(19000);
  check(lampOn && ring.shownBrightness > 0, "19초 뒤: 켜짐 (설정 20초)");
  run(3000);
  check(!lampOn && ring.shownBrightness == 0, "22초 뒤: 서서히 꺼져 완전히 꺼짐");
  run(300);
  check(rep("power") == "OFF" && rep("dark") == "50" && rep("off") == "20", "보고: power=OFF · dark=50 · off=20");

  section("6. 마지막 색·밝기 기억");
  sim::appSet("TEAM05", "ON"); run(2100 + 1100);
  check(ring.shownBrightness == MAX_BRIGHTNESS * 80 / 100 && rep("color") == "rainbow", "다시 켜면 무지개 · 80% 그대로");

  return finish("V2 앱 연동");
}
