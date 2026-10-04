/*
  5단계 — 스마트 보관함 (서보 4개 + 리드스위치 4개, 센서쉴드 버전)
  ----------------------------------------------------------------------
  슬롯마다 서보(잠금)와 리드스위치(우산 감지)가 짝을 이룹니다.
    리드스위치 감지(자석 가까이) = 우산 있음 / 감지 안 됨 = 우산 없음

  ● 대여: 열림 → 우산이 빠지면 5초 뒤 닫힘 → 확인
       우산 없음 → 대여완료
       30초가 지나도 안 가져가면 → 닫고 대여실패 + 서버에 대여 취소 요청
  ● 반납: 열림 → 우산이 꽂히면 "즉시" 닫힘 → 확인 (잡아주는 기구가 없어서)
       우산 감지 → 반납완료
       감지 안 됨 → 다시 열림 (최대 3회, 그래도 없으면 반납실패)

  기다리는 동안에도 멈추지 않습니다(논블로킹). 통신은 별도 코어에서 돌아가므로
  WiFi가 느려도 센서 감지와 5초 타이머가 밀리지 않습니다.

  명령 두 가지 경로
    ① 앱  : 서버 명령 큐를 1.5초마다 확인 ("RENT:L001:1" / "RETURN:L001:1")
            보관함 번호가 붙어 있어서 다른 보관함 명령은 집어오지 않습니다
    ② USB : 시리얼 모니터(115200)
            r1~r4 대여 · b1~b4 반납 · o1~o4 그냥 열기 · c1~c4 그냥 닫기 · s 상태
  ※ 서보가 안 움직이면 ENABLE_WIFI를 false로 두고 ②만 써보세요 — 전원 문제인지 바로 갈립니다

  배선: docs/wiring_servo_reed.md — 센서쉴드에 그대로 꽂으면 됩니다
    서보 신호: GPIO16·17·25·26 (실드 숫자 5·4·3·2)
    리드:      GPIO13·14·27·4  (실드 숫자 9·7·6·A1)
  ⚠ 서보 전원은 실드 외부 전원 단자에 5V — 12V를 넣으면 서보가 탑니다!

  라이브러리: ESP32Servo (기본 Servo.h는 ESP32에서 안 됩니다)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// ─────────────────────────────────────────────────────────────
// 1. 바꿀 곳
// ─────────────────────────────────────────────────────────────

// ⚠ WiFi 비밀번호를 넣은 채로 GitHub에 올리지 마세요!
const char WIFI_SSID[]     = "WiFi이름";
const char WIFI_PASSWORD[] = "비밀번호";

// 구글 시트 '시트1' 탭의 /exec 주소
const char SERVER_URL[] = "https://script.google.com/macros/s/XXXX/exec";

// 이 보드가 담당하는 보관함 — 시트 lockers 탭의 locker_id와 같아야 합니다.
// 슬롯 번호(1~4)는 보관함마다 겹치므로 이 값으로 구분합니다.
const char LOCKER_ID[] = "L001";

// WiFi를 아예 끄고 시리얼 명령만 쓰는 모드 (false로 바꾸기)
//   ① 서보가 안 움직일 때 원인 가리기 — test_slot1처럼 WiFi 없는 조건이 됩니다.
//      여기서 잘 움직이면 코드가 아니라 "WiFi가 켜지면 전원이 부족한 것"입니다.
//   ② 인터넷이 없는 발표장에서 시리얼로만 시연할 때
constexpr bool ENABLE_WIFI = true;

// ─────────────────────────────────────────────────────────────
// 2. 하드웨어 설정
// ─────────────────────────────────────────────────────────────

constexpr uint8_t SLOT_COUNT = 4;
constexpr uint8_t SERVO_PINS[SLOT_COUNT] = {16, 17, 25, 26};
constexpr uint8_t REED_PINS[SLOT_COUNT]  = {13, 14, 27, 4};

// 리드스위치를 GPIO-GND 사이에 연결하면 우산이 있을 때 LOW입니다.
// 반대로 동작하는 센서 모듈이면 HIGH로 바꾸세요.
constexpr uint8_t REED_ACTIVE_LEVEL = LOW;

// 이 기구는 실측으로 확인한 값입니다: 180도=잠김, 0도=열림 (기구 방향에 따라 다름)
// 끝단 각도라 서보가 스토퍼에 눌린 채 힘을 쓸 수 있지만, 동작 후 힘을 빼므로(detach)
// 계속 전류를 먹지는 않습니다. RELEASE_WHEN_IDLE을 끄면 발열이 생길 수 있어요.
constexpr int SERVO_CLOSED_ANGLE = 180;     // 잠김
constexpr int SERVO_OPEN_ANGLE   = 0;       // 열림
constexpr int SERVO_MIN_PULSE_US = 500;
constexpr int SERVO_MAX_PULSE_US = 2400;

// 다 움직인 뒤 서보의 힘을 뺄지(detach) — 전류·떨림·발열은 줄지만,
// ⚠ ESP32Servo 일부 버전은 detach할 때 PWM 채널을 반납하지 않습니다.
//   그러면 다음에 attach가 실패해서 "열림"만 찍히고 서보는 안 움직여요.
//   (실제로 겪은 증상: 부팅 때 네 칸을 붙였다 떼고 나면 그다음 명령부터 전부 실패)
//   그래서 기본은 false — setup에서 한 번만 붙이고 계속 붙여둡니다.
//   서보가 뜨겁거나 계속 떠는 게 더 문제라면 true로 바꿔보되, 위 증상을 확인하세요.
constexpr bool RELEASE_WHEN_IDLE = false;

// ─────────────────────────────────────────────────────────────
// 3. 시간 설정
// ─────────────────────────────────────────────────────────────

constexpr uint32_t RENT_CLOSE_DELAY_MS    = 5000;   // 우산이 빠진 뒤 닫기까지
constexpr uint32_t RETURN_PRESENT_HOLD_MS = 0;      // 우산이 감지되면 바로 닫음
// ↑ 우산을 잡아주는 기구가 없어서 감지 즉시 닫습니다. 리드 값은 이미 80ms
//   디바운스를 거치므로 순간 노이즈로 닫히지는 않아요. 너무 민감하면 300으로.
constexpr uint32_t SERVO_MOVE_MS          = 900;    // 서보가 다 움직일 때까지
constexpr uint32_t REED_DEBOUNCE_MS       = 80;     // 접점 떨림 무시
constexpr uint8_t  RETURN_RETRY_MAX       = 3;      // 반납 재개방 최대 횟수

// 열어둔 채 아무 일도 없을 때 자동으로 닫는 시간 (0이면 자동 닫힘 없음)
// 부스에서 누가 누르고 그냥 가버려도 문이 계속 열려 있지 않게 합니다.
constexpr uint32_t OPEN_FAILSAFE_MS = 30000;

constexpr uint32_t COMMAND_POLL_MS = 1500;   // 서버 명령 확인 주기
constexpr uint32_t REPORT_EVERY_MS = 30000;  // 슬롯 상태 정기 보고
constexpr uint32_t WIFI_RETRY_MS   = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS = 10000;

// ─────────────────────────────────────────────────────────────
// 4. 상태 정의
// ─────────────────────────────────────────────────────────────

enum class FlowState : uint8_t {
  LOCKED_IDLE,          // 잠긴 채 대기
  MANUAL_OPEN,          // o 명령으로 열어둔 상태
  RENT_WAIT_REMOVE,     // 대여: 열고 우산이 빠지기를 기다림
  RENT_VERIFY,          // 대여: 닫고 결과 확인
  RETURN_WAIT_INSERT,   // 반납: 열고 우산이 꽂히기를 기다림
  RETURN_WAIT_DONE,     // 반납(2단계): RETURN_DONE 명령을 기다림
  RETURN_VERIFY         // 반납: 닫고 결과 확인
};

struct SlotRuntime {
  FlowState state;
  uint32_t  stateSince;

  bool     servoOpen;
  bool     servoBusy;        // 움직이는 중 (다 움직이면 힘을 뺌)
  uint32_t servoMovedAt;

  bool     rawPresent;       // 디바운스 전 값
  bool     present;          // 디바운스 후 값
  uint32_t rawChangedAt;

  bool     absentTimerRunning;
  uint32_t absentSince;
  bool     presentTimerRunning;
  uint32_t presentSince;

  bool    returnAuto;        // 반납이 1단계(자동) 방식인지
  uint8_t returnAttempts;    // 반납 재개방 횟수
};

struct OutboundMessage { char query[192]; };   // 서버로 보낼 요청

Servo servos[SLOT_COUNT];
SlotRuntime slots[SLOT_COUNT];
QueueHandle_t commandQueue  = nullptr;
QueueHandle_t outboundQueue = nullptr;

struct CommandMessage { char text[64]; };

// millis()가 한 바퀴 돌아도(약 49일) 안전한 경과 시간 비교
bool hasElapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

// ─────────────────────────────────────────────────────────────
// 5. 서보와 리드스위치
// ─────────────────────────────────────────────────────────────

bool readReed(uint8_t i) { return digitalRead(REED_PINS[i]) == REED_ACTIVE_LEVEL; }

// 서보는 붙어 있는 동안 계속 힘을 씁니다. 움직일 때만 붙이고 끝나면 뗍니다.
// attach가 실패하면 write는 조용히 아무 일도 안 합니다. 서보는 전혀 안 움직이는데
// 시리얼에는 "열림"만 찍히는 상황이 되므로, 실패를 반드시 알려줍니다.
bool attachServo(uint8_t i) {
  if (servos[i].attached()) return true;
  servos[i].setPeriodHertz(50);
  if (servos[i].attach(SERVO_PINS[i], SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US) != 0)
    return true;
  Serial.printf("⚠ 슬롯%u 서보 연결 실패 — GPIO%u에 PWM을 못 잡았어요\n",
                i + 1, SERVO_PINS[i]);
  if (RELEASE_WHEN_IDLE)
    Serial.println("  RELEASE_WHEN_IDLE을 false로 두면(기본값) 이 문제가 없어집니다");
  return false;
}

void moveServo(uint8_t i, bool open) {
  attachServo(i);
  servos[i].write(open ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE);
  slots[i].servoOpen    = open;
  slots[i].servoBusy    = true;
  slots[i].servoMovedAt = millis();
}

// 다 움직인 서보의 힘 빼기 (매 루프에서 확인)
void updateServoPower(uint8_t i, uint32_t now) {
  if (!slots[i].servoBusy) return;
  if (!hasElapsed(now, slots[i].servoMovedAt, SERVO_MOVE_MS)) return;
  slots[i].servoBusy = false;
  if (RELEASE_WHEN_IDLE && servos[i].attached()) servos[i].detach();
}

void setState(uint8_t i, FlowState state, uint32_t now) {
  slots[i].state               = state;
  slots[i].stateSince          = now;
  slots[i].absentTimerRunning  = false;
  slots[i].presentTimerRunning = false;
}

void updateReedSwitches(uint32_t now) {
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    const bool raw = readReed(i);
    if (raw != slots[i].rawPresent) {
      slots[i].rawPresent   = raw;
      slots[i].rawChangedAt = now;
    }
    if (raw != slots[i].present &&
        hasElapsed(now, slots[i].rawChangedAt, REED_DEBOUNCE_MS)) {
      slots[i].present = raw;
    }
  }
}

// ─────────────────────────────────────────────────────────────
// 6. 서버로 보낼 요청 (네트워크 태스크가 실제로 전송)
// ─────────────────────────────────────────────────────────────

void queueRequest(const String& query) {
  if (outboundQueue == nullptr) return;
  OutboundMessage msg{};
  query.toCharArray(msg.query, sizeof(msg.query));
  if (xQueueSend(outboundQueue, &msg, 0) != pdTRUE)
    Serial.println("보낼 요청이 밀렸어요 (큐 가득)");
}

// 대여 취소 — 문은 열렸는데 우산을 안 가져갔을 때 요금이 청구되지 않도록
void requestCancel(uint8_t i) {
  queueRequest(String("?action=cancel&locker_id=") + LOCKER_ID +
               "&slot=" + String(i + 1));
}

// 반납 실패 — 서버는 '반납' 요청을 받은 순간 이미 반납 완료로 적고 정산까지
// 끝냅니다. 우산이 실제로 안 들어왔으면 이걸 보내 되돌려야, 우산은 손에 있는데
// 시트에는 반납된 걸로 남아 다음 사람에게 빈 칸이 열리는 일이 없습니다.
void requestReturnFailed(uint8_t i) {
  queueRequest(String("?action=return_failed&locker_id=") + LOCKER_ID +
               "&slot=" + String(i + 1));
}

// 네 칸의 우산 유무 보고는 "가장 최신 상태 한 번"이면 충분해서 큐에 쌓지 않고
// 깃발만 세운다. 통신이 안 되던 동안 보고가 밀려 실패 메시지가 쏟아지지 않는다.
volatile bool reportPending = false;
void requestReport() { reportPending = true; }

// 통신이 살아 있는지 보여주는 값들 — 시리얼 s(상태) · n(지금 물어보기)로 확인합니다.
// 지금까지는 명령 확인이 계속 실패해도 시리얼에 아무것도 안 나와서,
// "WiFi 연결됨"만 보고 정상이라고 착각하기 쉬웠습니다.
volatile uint32_t netLastOkMs   = 0;      // 마지막으로 서버가 응답한 시각
volatile uint32_t netPollCount  = 0;      // 명령을 확인한 횟수
volatile uint32_t netFailStreak = 0;      // 연속 실패 횟수
volatile int      netLastCode   = 0;      // 마지막 응답 코드
volatile bool     pingRequested = false;  // n 명령 — 지금 바로 한 번 물어보기

String buildReportQuery() {
  String q = String("?action=report&locker_id=") + LOCKER_ID;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    q += "&p" + String(i + 1) + "=" + String(slots[i].present ? 1 : 0);
  return q;
}

// ─────────────────────────────────────────────────────────────
// 7. 대여 · 반납 흐름
// ─────────────────────────────────────────────────────────────

void lockSlot(uint8_t i) {
  moveServo(i, false);
  setState(i, FlowState::LOCKED_IDLE, millis());
}

void startRent(uint8_t i) {
  const uint32_t now = millis();

  if (!slots[i].present) {          // 빌려줄 우산이 없음
    // 서보를 아예 안 움직이고 끝나는 유일한 경로입니다. 앱에서 눌렀는데
    // "아무 반응이 없다"면 십중팔구 여기예요 — 그래서 리드스위치 원시값까지 찍어
    // 배선 문제인지 자석 위치 문제인지 바로 가릴 수 있게 합니다.
    Serial.printf("슬롯%u 대여불가 — 우산이 없어요 (리드 GPIO%u=%s, 감지 기준=%s)\n",
                  i + 1, REED_PINS[i],
                  digitalRead(REED_PINS[i]) == HIGH ? "HIGH" : "LOW",
                  REED_ACTIVE_LEVEL == LOW ? "LOW" : "HIGH");
    Serial.println("  자석을 리드에 1cm 안으로 붙이고 s 를 눌러 다시 확인해보세요");
    requestCancel(i);               // 서버의 대여 기록도 되돌립니다
    return;
  }

  Serial.printf("슬롯%u 열림\n", i + 1);
  moveServo(i, true);
  setState(i, FlowState::RENT_WAIT_REMOVE, now);
}

void startReturn(uint8_t i, bool autoMode) {
  const uint32_t now = millis();
  Serial.printf("슬롯%u 열림\n", i + 1);
  moveServo(i, true);
  slots[i].returnAuto     = autoMode;
  slots[i].returnAttempts = 1;
  setState(i, autoMode ? FlowState::RETURN_WAIT_INSERT
                       : FlowState::RETURN_WAIT_DONE, now);
}

void updateSlotState(uint8_t i, uint32_t now) {
  SlotRuntime& s = slots[i];

  switch (s.state) {
    case FlowState::LOCKED_IDLE:
      break;

    case FlowState::MANUAL_OPEN:
      if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
        Serial.printf("슬롯%u 닫힘 (자동)\n", i + 1);
        lockSlot(i);
      }
      break;

    case FlowState::RENT_WAIT_REMOVE:
      if (!s.present) {                       // 우산을 가져갔다
        if (!s.absentTimerRunning) {
          s.absentTimerRunning = true;
          s.absentSince = now;
        }
        if (hasElapsed(now, s.absentSince, RENT_CLOSE_DELAY_MS)) {
          Serial.printf("%lu초후 닫힘\n", RENT_CLOSE_DELAY_MS / 1000);
          moveServo(i, false);
          setState(i, FlowState::RENT_VERIFY, now);
        }
      } else {
        s.absentTimerRunning = false;         // 다시 넣었으면 카운트 취소
        // 계속 안 가져가면 자동으로 닫고 실패 처리
        if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
          Serial.println("닫힘 (시간 초과)");
          moveServo(i, false);
          setState(i, FlowState::RENT_VERIFY, now);
        }
      }
      break;

    case FlowState::RENT_VERIFY:
      if (!hasElapsed(now, s.stateSince, SERVO_MOVE_MS)) break;
      if (!s.present) {
        Serial.println("대여완료");
      } else {
        Serial.println("대여실패 — 우산이 그대로 있어요");
        requestCancel(i);                     // 요금이 청구되지 않도록
      }
      setState(i, FlowState::LOCKED_IDLE, now);
      requestReport();
      break;

    case FlowState::RETURN_WAIT_INSERT:
      if (s.present) {                        // 우산이 꽂혔다
        if (!s.presentTimerRunning) {
          s.presentTimerRunning = true;
          s.presentSince = now;
        }
        if (hasElapsed(now, s.presentSince, RETURN_PRESENT_HOLD_MS)) {
          Serial.println("닫힘");
          moveServo(i, false);
          setState(i, FlowState::RETURN_VERIFY, now);
        }
      } else {
        s.presentTimerRunning = false;
        if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
          Serial.println("반납실패 — 우산이 안 들어왔어요");
          lockSlot(i);
          requestReturnFailed(i);
          requestReport();
        }
      }
      break;

    case FlowState::RETURN_WAIT_DONE:         // 2단계 반납: 닫기 명령 대기
      if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
        Serial.println("반납실패 — 시간이 지났어요");
        lockSlot(i);
        requestReturnFailed(i);
        requestReport();
      }
      break;

    case FlowState::RETURN_VERIFY:
      if (!hasElapsed(now, s.stateSince, SERVO_MOVE_MS)) break;
      if (s.present) {
        Serial.println("반납완료");
        setState(i, FlowState::LOCKED_IDLE, now);
        requestReport();
      } else if (s.returnAttempts >= RETURN_RETRY_MAX) {
        Serial.printf("반납실패 — %u번 열어도 우산이 없어요\n", RETURN_RETRY_MAX);
        setState(i, FlowState::LOCKED_IDLE, now);
        requestReturnFailed(i);
        requestReport();
      } else {
        s.returnAttempts++;                   // 다시 한 번 기회
        Serial.println("우산이 없어요 — 다시 열게요");
        Serial.printf("슬롯%u 열림\n", i + 1);
        moveServo(i, true);
        setState(i, s.returnAuto ? FlowState::RETURN_WAIT_INSERT
                                 : FlowState::RETURN_WAIT_DONE, now);
      }
      break;
  }
}

// ─────────────────────────────────────────────────────────────
// 8. 명령 해석 (앱·시리얼 공용)
// ─────────────────────────────────────────────────────────────

void printStatus() {
  Serial.println("── 슬롯 상태 ──");
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    Serial.printf("  슬롯%u: 우산 %s  (리드 GPIO%u=%s)\n", i + 1,
                  slots[i].present ? "있음" : "없음", REED_PINS[i],
                  digitalRead(REED_PINS[i]) == HIGH ? "HIGH" : "LOW");

  Serial.println("── 통신 상태 ──");
  if (!ENABLE_WIFI) { Serial.println("  WiFi 꺼둠 (ENABLE_WIFI = false)"); return; }
  const bool up = (WiFi.status() == WL_CONNECTED);
  Serial.printf("  WiFi: %s\n", up ? "연결됨" : "끊김");
  if (netPollCount == 0) {
    Serial.println("  서버: 아직 한 번도 확인 못 함 ← 여기서 막혀 있으면 앱 명령이 안 옵니다");
  } else {
    Serial.printf("  서버: 확인 %lu회 · 마지막 응답 %d · 연속실패 %lu회\n",
                  (unsigned long)netPollCount, netLastCode, (unsigned long)netFailStreak);
    if (netLastOkMs > 0)
      Serial.printf("  마지막 성공: %lu초 전\n",
                    (unsigned long)((millis() - netLastOkMs) / 1000));
    else
      Serial.println("  성공한 적 없음 ← 배포 URL·와이파이를 확인하세요");
  }
  Serial.println("  (n 을 누르면 지금 바로 서버에 한 번 물어봅니다)");
}

bool runSlotCommand(String verb, int slotNumber) {
  verb.trim();
  verb.toUpperCase();
  verb.replace('-', '_');

  if (slotNumber < 1 || slotNumber > SLOT_COUNT) {
    Serial.println("슬롯 번호는 1~4 입니다");
    return true;
  }
  const uint8_t i = static_cast<uint8_t>(slotNumber - 1);

  if (verb == "RENT")                                   { startRent(i); }
  else if (verb == "RETURN")                            { startReturn(i, true); }
  else if (verb == "RETURN_OPEN" || verb == "RETURN_START") { startReturn(i, false); }
  else if (verb == "RETURN_DONE" || verb == "RETURN_COMPLETE" || verb == "RETURN_CLOSE") {
    if (slots[i].state == FlowState::RETURN_WAIT_DONE) {
      Serial.println("닫힘");
      moveServo(i, false);
      setState(i, FlowState::RETURN_VERIFY, millis());
    } else {
      startReturn(i, true);        // 열기 명령을 놓쳤으면 자동 방식으로 복구
    }
  }
  else if (verb == "OPEN" || verb == "UNLOCK") {
    Serial.printf("슬롯%d 열림 (닫으려면 c%d)\n", slotNumber, slotNumber);
    moveServo(i, true);
    setState(i, FlowState::MANUAL_OPEN, millis());
  }
  else if (verb == "CLOSE" || verb == "LOCK") {
    Serial.printf("슬롯%d 닫힘\n", slotNumber);
    lockSlot(i);
  }
  else return false;

  return true;
}

bool isNumber(const String& v) {
  if (v.length() == 0) return false;
  for (size_t i = 0; i < v.length(); ++i) if (!isDigit(v[i])) return false;
  return true;
}

void executeCommand(String command) {
  command.trim();
  command.toUpperCase();
  command.replace('-', '_');
  if (command.length() == 0) return;

  // 시리얼 단축 명령: s / r1 / b1 / o1 / c1
  if (command == "S") { printStatus(); return; }
  if (command == "N") {                      // 지금 바로 서버에 물어보기 (진단용)
    if (!ENABLE_WIFI) { Serial.println("ENABLE_WIFI = false 라 서버를 안 씁니다"); return; }
    Serial.println("서버에 물어보는 중...");
    pingRequested = true;                    // 실제 통신은 네트워크 태스크가 합니다
    return;
  }
  if (command.length() == 2 && isDigit(command[1])) {
    const int slot = command[1] - '0';
    switch (command[0]) {
      case 'R': runSlotCommand("RENT", slot);   return;
      case 'B': runSlotCommand("RETURN", slot); return;
      case 'O': runSlotCommand("OPEN", slot);   return;
      case 'C': runSlotCommand("CLOSE", slot);  return;
      default: break;
    }
  }

  // 슬롯 없는 전체 명령
  if (command == "LOCK" || command == "CLOSE") {
    for (uint8_t i = 0; i < SLOT_COUNT; ++i) lockSlot(i);
    Serial.println("전체 닫힘");
    return;
  }

  // "RENT:1" / "RENT:L001:1" / "L001:RENT:1"
  String parts[3];
  uint8_t count = 0;
  int start = 0;
  while (count < 3) {
    const int sep = command.indexOf(':', start);
    if (sep < 0) { parts[count++] = command.substring(start); break; }
    parts[count++] = command.substring(start, sep);
    start = sep + 1;
  }

  String verb, locker, slotText;
  if (count == 2) {
    verb = parts[0]; slotText = parts[1];
  } else if (count == 3) {
    // "L001:RENT:1"처럼 보관함이 앞에 오는 형식도 받아줍니다
    if (parts[0].equalsIgnoreCase(LOCKER_ID)) { locker = parts[0]; verb = parts[1]; slotText = parts[2]; }
    else                                      { verb = parts[0]; locker = parts[1]; slotText = parts[2]; }
  } else {
    Serial.println("모르는 명령: " + command +
                   "  (r1~r4=대여, b1~b4=반납, o1~o4=열기, c1~c4=닫기, s=상태)");
    return;
  }

  locker.trim();
  if (locker.length() > 0 && !locker.equalsIgnoreCase(LOCKER_ID)) {
    Serial.println("다른 보관함 명령이라 무시: " + locker);
    return;
  }

  slotText.trim();
  if (!isNumber(slotText) || !runSlotCommand(verb, slotText.toInt()))
    Serial.println("모르는 명령: " + command);
}

// ─────────────────────────────────────────────────────────────
// 9. 네트워크 (별도 코어에서 실행 — 센서 타이밍을 방해하지 않게)
// ─────────────────────────────────────────────────────────────

// 자주 나오는 오류 코드에 원인 설명을 붙여준다
String explainError(int code) {
  if (code == -1)  return " — 서버까지 연결이 안 됨. 이 와이파이가 인터넷이 되는지 확인!"
                          " (핫스팟이면 데이터 켜기, 로그인 필요한 공유기는 사용 불가)";
  if (code == -11) return " — 응답이 늦음 (회선이 느리거나 서버가 깨어나는 중)";
  if (code == 404) return " — 주소 없음 (URL이 시트의 /exec 주소인지 확인)";
  if (code == 401 || code == 403) return " — 권한 (배포 액세스를 '모든 사용자'로)";
  return "";
}

int httpsGet(const String& url, String& body) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();          // 시제품용. 운영에서는 루트 인증서를 등록하세요.
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) return -1000;
  const int code = http.GET();
  if (code > 0) body = http.getString();
  http.end();
  client.stop();
  return code;
}

// 서버는 "RENT:1" 같은 평문을 돌려줍니다. 나중에 JSON으로 바뀌어도 읽히게 해둡니다.
String decodeCommand(String body) {
  body.trim();
  if (body.length() == 0 || body == "null") return "";

  if (body.startsWith("{")) {
    const char* keys[] = {"\"command\"", "\"cmd\""};
    for (const char* key : keys) {
      const int keyPos = body.indexOf(key);
      if (keyPos < 0) continue;
      const int colon = body.indexOf(':', keyPos + strlen(key));
      if (colon < 0) continue;
      const int q1 = body.indexOf('"', colon + 1);
      if (q1 < 0) continue;
      int q2 = q1 + 1;
      while (q2 < static_cast<int>(body.length())) {
        if (body[q2] == '"' && body[q2 - 1] != '\\') break;
        ++q2;
      }
      if (q2 < static_cast<int>(body.length())) { body = body.substring(q1 + 1, q2); break; }
    }
  } else if (body.length() >= 2 && body[0] == '"' && body[body.length() - 1] == '"') {
    body = body.substring(1, body.length() - 1);
  }

  body.replace("\\\"", "\"");
  body.replace("\\n", "");
  body.replace("\\r", "");
  body.trim();
  return body;
}

void networkTask(void* parameter) {
  (void)parameter;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t lastWiFiTry = millis();
  uint32_t lastPoll    = millis() - COMMAND_POLL_MS;
  uint32_t lastReport  = millis();
  bool wasConnected    = false;

  for (;;) {
    const uint32_t now = millis();
    const bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected != wasConnected) {
      wasConnected = connected;
      Serial.println(connected ? "WiFi 연결됨 — 앱 명령 대기"
                               : "WiFi 끊김 — USB 명령만 동작합니다");
      if (connected) {
        // 연결되자마자 서버가 진짜 닿는지 한 번 확인해서 알려준다
        String body;
        const int code = httpsGet(String(SERVER_URL) +
                                  "?action=cmd&probe=1&locker_id=" + LOCKER_ID, body);
        if (code == HTTP_CODE_OK) {
          Serial.println("서버 연결 확인 완료");
          if (body.length() > 0) {          // 밀려 있던 명령이면 바로 실행
            CommandMessage msg{};
            decodeCommand(body).toCharArray(msg.text, sizeof(msg.text));
            if (strlen(msg.text) > 0) xQueueSend(commandQueue, &msg, 0);
          }
        } else {
          Serial.println("서버 연결 안 됨 (" + String(code) + ")" + explainError(code));
        }
        requestReport();
      }
    }

    if (!connected) {
      if (hasElapsed(now, lastWiFiTry, WIFI_RETRY_MS)) {
        lastWiFiTry = now;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // n 명령 — 지금 바로 서버에 물어보고, 무엇이 오는지 그대로 보여줍니다
    if (pingRequested) {
      pingRequested = false;
      String body;
      const uint32_t t0 = millis();
      const int code = httpsGet(String(SERVER_URL) +
                                "?action=cmd&probe=1&locker_id=" + LOCKER_ID, body);
      Serial.printf("  응답 코드 %d (%lums)\n", code, (unsigned long)(millis() - t0));
      if (code == HTTP_CODE_OK) {
        netLastOkMs = millis();
        netFailStreak = 0;
        const String command = decodeCommand(body);
        if (command.length() == 0) {
          Serial.println("  대기 중인 명령 없음 — 서버와 통신은 정상입니다");
          Serial.println("  (앱에서 대여를 누른 직후에 n 을 눌러 명령이 오는지 보세요)");
        } else {
          Serial.println("  받은 명령: " + command + " — 바로 실행합니다");
          CommandMessage msg{};
          command.toCharArray(msg.text, sizeof(msg.text));
          xQueueSend(commandQueue, &msg, 0);
        }
      } else {
        Serial.println("  실패" + explainError(code));
      }
    }

    // 보낼 요청이 있으면 먼저 처리 (대여 취소가 늦으면 안 되니까)
    OutboundMessage out{};
    if (xQueueReceive(outboundQueue, &out, 0) == pdTRUE) {
      String body;
      const int code = httpsGet(String(SERVER_URL) + out.query, body);
      if (code != HTTP_CODE_OK)
        Serial.println("서버 전송 실패 (" + String(code) + ")" + explainError(code));
    }
    else if (reportPending) {              // 상태 보고는 최신 것 한 번만
      reportPending = false;
      String body;
      const int code = httpsGet(String(SERVER_URL) + buildReportQuery(), body);
      if (code != HTTP_CODE_OK) {
        static bool noticed = false;       // 같은 실패를 계속 떠들지 않는다
        if (!noticed) {
          Serial.println("현황 보고 실패 (" + String(code) + ")" + explainError(code));
          Serial.println("  (연결이 회복되면 자동으로 다시 보냅니다)");
          noticed = true;
        }
        reportPending = true;              // 다음 기회에 다시
        vTaskDelay(pdMS_TO_TICKS(5000));   // 실패 직후 재시도 홍수 방지
      }
    }

    // 큐에 자리가 있을 때만 명령을 꺼냅니다 — 받아놓고 잃어버리지 않게
    if (hasElapsed(now, lastPoll, COMMAND_POLL_MS) &&
        uxQueueSpacesAvailable(commandQueue) > 0) {
      lastPoll = now;
      String body;
      const String url = String(SERVER_URL) + "?action=cmd&locker_id=" + LOCKER_ID +
                         "&_=" + String(now);
      const int code = httpsGet(url, body);
      netPollCount++;
      netLastCode = code;
      if (code == HTTP_CODE_OK) {
        if (netFailStreak > 0) {
          Serial.printf("명령 확인 회복됨 (%lu회 실패 뒤)\n", (unsigned long)netFailStreak);
          netFailStreak = 0;
        }
        netLastOkMs = millis();
        const String command = decodeCommand(body);
        if (command.length() > 0) {
          CommandMessage msg{};
          command.toCharArray(msg.text, sizeof(msg.text));
          xQueueSend(commandQueue, &msg, 0);
        }
      } else {
        // 여기서 조용히 넘어가면 "WiFi는 연결됐는데 앱 명령만 안 오는" 상태가 됩니다.
        // 첫 실패와 그 뒤 30초마다(20회) 한 번씩만 알려 시리얼을 덮지 않게 합니다.
        netFailStreak++;
        if (netFailStreak == 1 || netFailStreak % 20 == 0)
          Serial.println("명령 확인 실패 " + String((unsigned long)netFailStreak) +
                         "회 (" + String(code) + ")" + explainError(code));
      }
    }

    if (hasElapsed(now, lastReport, REPORT_EVERY_MS)) {
      lastReport = now;
      requestReport();
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ─────────────────────────────────────────────────────────────
// 10. setup / loop
// ─────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== 스마트 우산 보관함 ===");

  commandQueue  = xQueueCreate(8, sizeof(CommandMessage));
  outboundQueue = xQueueCreate(12, sizeof(OutboundMessage));
  if (commandQueue == nullptr || outboundQueue == nullptr) {
    Serial.println("메모리 부족 — 다시 켜주세요");
    while (true) delay(1000);
  }

  // 네 칸을 한 개씩 차례로 잠급니다 — 동시에 움직이면 전류가 확 튀어
  // 약한 전원에서는 보드가 리셋됩니다.
  // ★ ESP32Servo는 쓸 수 있는 PWM 타이머를 미리 잡아두어야 합니다.
  //   이게 없으면 라이브러리 버전에 따라 attach()가 조용히 실패해서
  //   "열림"은 찍히는데 서보가 하나도 안 움직입니다. 서보 4개면 특히 잘 걸려요.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  Serial.println("네 칸을 차례로 잠급니다...");
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    pinMode(REED_PINS[i], INPUT_PULLUP);

    const bool present = readReed(i);
    slots[i] = SlotRuntime{};
    slots[i].state        = FlowState::LOCKED_IDLE;
    slots[i].rawPresent   = present;
    slots[i].present      = present;
    slots[i].rawChangedAt = millis();
    slots[i].stateSince   = millis();

    // 여기서 한 번 붙여두고 계속 씁니다 — 붙였다 뗐다 하면 채널이 모자라
    // 다음 attach가 실패할 수 있어서(위 RELEASE_WHEN_IDLE 설명 참고)
    attachServo(i);
    Serial.printf("슬롯%u 잠금\n", i + 1);
    moveServo(i, false);
    delay(SERVO_MOVE_MS);                       // 다 움직일 때까지 기다렸다가
    slots[i].servoBusy = false;
    if (RELEASE_WHEN_IDLE) servos[i].detach();  // 힘 빼고 다음 칸으로
  }

  printStatus();
  Serial.println("시리얼 명령");
  Serial.println("  r1~r4 = 대여   b1~b4 = 반납   s = 슬롯·통신 상태   n = 서버 확인");
  Serial.println("  o1~o4 = 그냥 열기   c1~c4 = 그냥 닫기  (서보·전원 점검용)");

  if (!ENABLE_WIFI) {
    Serial.println("WiFi 꺼둠 — 시리얼 명령만 동작합니다 (ENABLE_WIFI = false)");
    return;
  }
  if (xTaskCreatePinnedToCore(networkTask, "umbrella-net", 8192,
                              nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("네트워크 시작 실패 — USB 명령만 동작합니다");
  }
}

void loop() {
  const uint32_t now = millis();

  // ① USB(시리얼) 명령
  if (Serial.available()) executeCommand(Serial.readStringUntil('\n'));

  // ② 앱에서 온 명령
  CommandMessage msg{};
  while (xQueueReceive(commandQueue, &msg, 0) == pdTRUE)
    executeCommand(String(msg.text));

  // ③ 센서 읽기 · 상태 진행 · 서보 힘 빼기
  updateReedSwitches(now);
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    updateSlotState(i, now);
    updateServoPower(i, now);
  }

  delay(5);
}
