// ─────────────────────────────────────────────────────────────
//  step5_locker.ino 시뮬레이터
//  보드 없이 PC에서 스케치를 "그대로" 컴파일해 돌려봅니다.
//  → 대여·반납 흐름, 타이머, 서보 각도, 서버로 나가는 요청을 눈으로 확인
//
//  실행:  make run     (또는 build.sh)
//
//  ⚠ 전기(전류·전압)는 흉내낼 수 없습니다. 이 시뮬레이터가 통과한다는 것은
//    "코드 로직에는 문제가 없다"는 뜻이지, 실물이 움직인다는 뜻은 아닙니다.
// ─────────────────────────────────────────────────────────────
#include "stubs/Arduino.h"
#include "stubs/ESP32Servo.h"
#include "stubs/WiFi.h"

namespace sim {
  uint32_t nowMs = 0;
  int      pinLevel[40];
  std::vector<std::string> serialOut;
  std::string serialIn;
  ServoState servoState[40];
  bool allocatedTimer[4] = {false, false, false, false};
  int  channelsUsed = 0;
}
SerialSim Serial;
WiFiSim   WiFi;

#include "../step5_locker.ino"      // ★ 진짜 스케치를 그대로 가져옵니다

// ─────────────────────────────────────────────────────────────
//  테스트 도구
// ─────────────────────────────────────────────────────────────
static int  failures = 0;
static bool traceServo = true;
static int  lastAngle[SLOT_COUNT];
static std::vector<std::string> sentQueries;   // 서버로 나간 요청 기록

static int servoAngle(uint8_t i) { return sim::servoState[SERVO_PINS[i]].angle; }

static void flush() {
  for (const auto& line : sim::serialOut)
    std::printf("   %8.2fs │ %s\n", sim::nowMs / 1000.0, line.c_str());
  sim::serialOut.clear();

  OutboundMessage out{};
  while (xQueueReceive(outboundQueue, &out, 0) == pdTRUE) {
    std::printf("   %8.2fs │ → 서버로 전송: %s\n", sim::nowMs / 1000.0, out.query);
    sentQueries.push_back(out.query);
  }

  if (traceServo)
    for (uint8_t i = 0; i < SLOT_COUNT; ++i)
      if (servoAngle(i) != lastAngle[i]) {
        std::printf("   %8.2fs │ ⚙ 슬롯%u 서보 %d° → %d°%s\n", sim::nowMs / 1000.0, i + 1,
                    lastAngle[i], servoAngle(i),
                    servoAngle(i) == SERVO_OPEN_ANGLE ? " (열림)" : " (잠김)");
        lastAngle[i] = servoAngle(i);
      }
}

static void advance(uint32_t ms) {          // 가상 시간을 흘려보내며 loop()를 돌립니다
  const uint32_t until = sim::nowMs + ms;
  while (sim::nowMs < until) { loop(); flush(); }
}

static void setUmbrella(uint8_t slot, bool present) {   // 리드스위치 상태 바꾸기
  sim::pinLevel[REED_PINS[slot]] = present ? REED_ACTIVE_LEVEL : !REED_ACTIVE_LEVEL;
  std::printf("   %8.2fs │ 🖐 슬롯%u 우산을 %s\n", sim::nowMs / 1000.0, slot + 1,
              present ? "꽂음" : "꺼냄");
}

static void serialCmd(const char* cmd) {
  std::printf("   %8.2fs │ ⌨  시리얼 입력: %s\n", sim::nowMs / 1000.0, cmd);
  sim::serialIn = cmd;
  loop(); flush();
}

static void appCmd(const char* cmd) {       // 서버 명령 큐에 들어온 것처럼
  std::printf("   %8.2fs │ 📱 앱 명령 도착: %s\n", sim::nowMs / 1000.0, cmd);
  CommandMessage msg{};
  std::snprintf(msg.text, sizeof(msg.text), "%s", cmd);
  xQueueSend(commandQueue, &msg, 0);
  loop(); flush();
}

static void title(const char* t) { std::printf("\n━━━ %s ━━━\n", t); }

static void expect(bool ok, const char* what) {
  std::printf("   %s %s\n", ok ? "✅" : "❌", what);
  if (!ok) failures++;
}

static void resetAll() {                    // 모든 슬롯을 우산 있음 + 잠김으로
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    sim::pinLevel[REED_PINS[i]] = REED_ACTIVE_LEVEL;
  advance(500);
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) lockSlot(i);
  advance(1500);
  sim::serialOut.clear();
  OutboundMessage o{};
  while (xQueueReceive(outboundQueue, &o, 0) == pdTRUE) {}
  sentQueries.clear();
  reportPending = false;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) lastAngle[i] = servoAngle(i);
}

// ─────────────────────────────────────────────────────────────
int main() {
  std::printf("╔══════════════════════════════════════════════════════════╗\n");
  std::printf("║  스마트 우산 보관함 — 스케치 시뮬레이션 (보드 없이 PC에서) ║\n");
  std::printf("╚══════════════════════════════════════════════════════════╝\n");
  std::printf("설정: 잠김 %d° · 열림 %d° · 대여 후 닫기 %lums · 자동닫힘 %lums · 재시도 %u회\n",
              SERVO_CLOSED_ANGLE, SERVO_OPEN_ANGLE,
              (unsigned long)RENT_CLOSE_DELAY_MS, (unsigned long)OPEN_FAILSAFE_MS,
              RETURN_RETRY_MAX);

  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    sim::pinLevel[REED_PINS[i]] = REED_ACTIVE_LEVEL;   // 네 칸 모두 우산 있음
    lastAngle[i] = -1;
  }

  title("0. 전원 켜기 (setup)");
  setup();
  flush();
  bool allLocked = true;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) allLocked &= (servoAngle(i) == SERVO_CLOSED_ANGLE);
  expect(allLocked, "네 칸이 모두 잠김 각도로 이동");
  bool asConfigured = true;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    asConfigured &= (sim::servoState[SERVO_PINS[i]].attached == !RELEASE_WHEN_IDLE);
  expect(asConfigured, RELEASE_WHEN_IDLE
                       ? "다 움직인 뒤 힘을 뺌 (RELEASE_WHEN_IDLE = true)"
                       : "붙인 채로 유지 — 다음 명령에서 다시 attach하지 않도록");
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) lastAngle[i] = servoAngle(i);

  // ───────────────────────────────────────────────────────────
  title("1. 대여 성공 — 우산이 있고, 꺼내 감");
  resetAll();
  serialCmd("r1");
  expect(servoAngle(0) == SERVO_OPEN_ANGLE, "슬롯1이 열림");
  advance(1000);
  setUmbrella(0, false);                    // 우산을 꺼냄
  advance(RENT_CLOSE_DELAY_MS - 1000);
  expect(servoAngle(0) == SERVO_OPEN_ANGLE, "5초가 되기 전에는 열려 있음");
  advance(1500);
  expect(servoAngle(0) == SERVO_CLOSED_ANGLE, "우산을 꺼낸 지 5초 뒤 잠김");
  advance(SERVO_MOVE_MS + 200);
  expect(slots[0].state == FlowState::LOCKED_IDLE, "대여완료 후 대기 상태로 복귀");

  // ───────────────────────────────────────────────────────────
  title("2. 대여 불가 — 그 칸에 우산이 없음  ⚠ 서보가 안 움직이는 유일한 경우");
  resetAll();
  setUmbrella(0, false);
  advance(300);
  const int before = servoAngle(0);
  serialCmd("r1");
  expect(servoAngle(0) == before, "서보가 전혀 안 움직임 (문을 열지 않음)");
  expect(sentQueries.size() == 1 &&
         sentQueries[0] == "?action=cancel&locker_id=L001&slot=1",
         "서버에 대여 취소를 보냄 (요금이 청구되지 않게)");
  flush();

  // ───────────────────────────────────────────────────────────
  title("3. 대여 타임아웃 — 열었는데 30초간 안 가져감");
  resetAll();
  serialCmd("r1");
  expect(servoAngle(0) == SERVO_OPEN_ANGLE, "슬롯1이 열림");
  advance(OPEN_FAILSAFE_MS - 2000);
  expect(servoAngle(0) == SERVO_OPEN_ANGLE, "30초 전까지는 열려 있음");
  advance(3000);
  expect(servoAngle(0) == SERVO_CLOSED_ANGLE, "30초가 지나 자동으로 잠김");
  advance(SERVO_MOVE_MS + 200);

  // ───────────────────────────────────────────────────────────
  title("4. 반납 성공 — 우산을 꽂으면 즉시 닫힘");
  resetAll();
  setUmbrella(1, false);                    // 슬롯2는 빌려나가 비어 있는 상태
  advance(300);
  serialCmd("b2");
  expect(servoAngle(1) == SERVO_OPEN_ANGLE, "슬롯2가 열림");
  advance(1000);
  setUmbrella(1, true);                     // 우산을 꽂음
  advance(300);                             // 디바운스(80ms) 후 즉시 닫힘
  expect(servoAngle(1) == SERVO_CLOSED_ANGLE, "감지되자마자 잠김 (기다리지 않음)");
  advance(SERVO_MOVE_MS + 200);
  expect(slots[1].state == FlowState::LOCKED_IDLE, "반납완료 후 대기 상태로 복귀");

  // ───────────────────────────────────────────────────────────
  title("5. 반납 재시도 — 닫고 보니 우산이 없어서 다시 열어줌 (최대 3회)");
  resetAll();
  setUmbrella(2, false);
  advance(300);
  serialCmd("b3");
  for (int attempt = 1; attempt <= 3; ++attempt) {
    advance(500);
    setUmbrella(2, true);                   // 꽂은 척 → 닫히는 동안 빠짐
    advance(300);
    setUmbrella(2, false);
    advance(SERVO_MOVE_MS + 300);
  }
  expect(slots[2].state == FlowState::LOCKED_IDLE, "3회를 다 쓰면 반납실패로 종료");
  expect(slots[2].returnAttempts == RETURN_RETRY_MAX, "재개방 횟수가 3회에서 멈춤");
  expect(sentQueries.size() == 1 &&
         sentQueries[0] == "?action=return_failed&locker_id=L001&slot=3",
         "서버에 반납 취소를 보냄 (반납·정산을 되돌리도록)");
  flush();

  // ───────────────────────────────────────────────────────────
  title("6. 반납 타임아웃 — 30초간 안 꽂음 → 서버에 반납 취소 요청");
  resetAll();
  setUmbrella(3, false);
  advance(300);
  serialCmd("b4");
  advance(OPEN_FAILSAFE_MS + 1000);
  expect(servoAngle(3) == SERVO_CLOSED_ANGLE, "시간이 지나 잠김");
  expect(sentQueries.size() == 1 &&
         sentQueries[0] == "?action=return_failed&locker_id=L001&slot=4",
         "서버에 반납 취소를 보냄");
  flush();

  // ───────────────────────────────────────────────────────────
  title("7. 앱 명령 — 내 보관함 것만 실행하고 남의 것은 무시");
  resetAll();
  appCmd("RENT:L002:2");                    // 다른 보관함 → 무시해야 함
  expect(servoAngle(1) == SERVO_CLOSED_ANGLE, "L002 명령에는 반응하지 않음");
  appCmd("RENT:L001:2");                    // 내 보관함 → 실행
  expect(servoAngle(1) == SERVO_OPEN_ANGLE, "L001 명령에는 슬롯2가 열림");
  advance(1000);
  appCmd("RENT:3");                         // 옛 형식(보관함 없음)도 동작
  expect(servoAngle(2) == SERVO_OPEN_ANGLE, "옛 형식 RENT:3 도 여전히 동작 (하위 호환)");
  advance(OPEN_FAILSAFE_MS + 2000);
  flush();

  // ───────────────────────────────────────────────────────────
  title("8. 오래 써도 계속 움직이나 — PWM 채널 고갈 재현");
  resetAll();
  std::printf("   (붙였다 뗐다 하면 채널이 말라 attach가 실패하는 라이브러리를 흉내냅니다)\n");
  bool everyCycleMoved = true;
  for (int cycle = 1; cycle <= 12; ++cycle) {
    traceServo = false;
    serialCmd("o1"); advance(SERVO_MOVE_MS + 100);
    if (servoAngle(0) != SERVO_OPEN_ANGLE) everyCycleMoved = false;
    serialCmd("c1"); advance(SERVO_MOVE_MS + 100);
    if (servoAngle(0) != SERVO_CLOSED_ANGLE) everyCycleMoved = false;
  }
  traceServo = true;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) lastAngle[i] = servoAngle(i);
  sim::serialOut.clear();
  expect(everyCycleMoved, "12번 여닫아도 매번 실제로 움직임 (채널이 마르지 않음)");
  std::printf("   ℹ 쓴 채널 %d개 / 한도 %d개 — 슬롯마다 한 번씩만 붙였다는 뜻\n",
              sim::channelsUsed, sim::CHANNEL_LIMIT);

  // ───────────────────────────────────────────────────────────
  title("9. PWM 타이머를 안 잡았을 때 (고치기 전 코드 재현)");
  resetAll();
  for (bool& t : sim::allocatedTimer) t = false;      // allocateTimer를 안 부른 상태
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) servos[i].detach();
  sim::channelsUsed = 0;
  lastAngle[0] = servoAngle(0);
  const int beforeNoTimer = servoAngle(0);
  serialCmd("o1");
  expect(servoAngle(0) == beforeNoTimer,
         "attach 실패 → '열림'은 찍히는데 서보는 안 움직임 (증상 재현)");
  ESP32PWM::allocateTimer(0); ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2); ESP32PWM::allocateTimer(3);
  serialCmd("o1");
  expect(servoAngle(0) == SERVO_OPEN_ANGLE, "타이머를 잡아주면 정상 동작");
  flush();

  std::printf("\n══════════════════════════════════════════════════════════\n");
  if (failures == 0) std::printf("결과: 전부 통과 — 코드 로직에는 문제가 없습니다.\n");
  else               std::printf("결과: %d개 실패\n", failures);
  std::printf("※ 전류·전압은 흉내낼 수 없습니다. 실물이 안 움직이면 전원·배선 쪽입니다.\n");
  return failures == 0 ? 0 : 1;
}
