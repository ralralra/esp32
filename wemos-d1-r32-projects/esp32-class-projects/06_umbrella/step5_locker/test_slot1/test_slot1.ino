/*
  1번 슬롯 단독 테스트 — 서보 1개 + 리드스위치 1개
  ----------------------------------------------------
  WiFi·서버 없이 배선과 부품만 확인하는 코드입니다.

  배선 (센서쉴드 기준)
    서보 1번   : 실드 5번 자리 (GPIO16) — 3핀 그대로, 주황=S
    리드스위치 : 실드 9번 자리 (GPIO13) — S와 G에만 연결
    ⚠ 서보 전원은 실드 외부 전원 단자에 5V!

  켜면 자동으로:
    - 자석을 대거나 떼면 즉시 "우산 있음/없음"이 찍힘 (리드 확인)

  시리얼 모니터(115200) 명령:
    o = 열기 (0도, 열린 채 유지 — 전압 재보기 좋음)
    c = 닫기 (180도)
    t = 대여 흐름 테스트: 열림 → 5초 → 닫힘 → 우산 유무로 결과 판정
    w = 왕복 10회 (열림↔닫힘 반복 — 전원이 버티는지 스트레스 테스트)
    s = 지금 상태 보기
*/

#include <ESP32Servo.h>

const int SERVO_PIN = 16;     // 실드 5번 자리
const int REED_PIN  = 13;     // 실드 9번 자리

const int ANGLE_CLOSED = 180;   // 실측 확인값
const int ANGLE_OPEN   = 0;

Servo servo;
bool lastPresent;

// 자석 가까이(우산 있음) → LOW
bool umbrellaPresent() { return digitalRead(REED_PIN) == LOW; }

void moveTo(int angle) {
  if (!servo.attached()) {
    if (servo.attach(SERVO_PIN, 500, 2400) == 0)
      Serial.printf("⚠ 서보 연결 실패 — GPIO%d에 PWM을 못 잡았어요\n", SERVO_PIN);
  }
  servo.write(angle);
  delay(900);                 // 다 움직일 때까지
  servo.detach();             // 힘 빼기 (전류·떨림 감소)
}

void printState() {
  Serial.printf("우산 %s\n", umbrellaPresent() ? "있음" : "없음");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(REED_PIN, INPUT_PULLUP);

  // ESP32Servo가 쓸 PWM 타이머 확보 — 없으면 attach()가 조용히 실패합니다
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo.setPeriodHertz(50);

  Serial.println("\n=== 1번 슬롯 테스트 ===");
  Serial.println("명령: o=열기  c=닫기  t=대여흐름  w=왕복10회  s=상태");

  Serial.println("닫힘(180도)으로 시작");
  moveTo(ANGLE_CLOSED);

  lastPresent = umbrellaPresent();
  printState();
  Serial.println("→ 자석을 대거나 떼보세요. 바뀔 때마다 찍힙니다.");
}

void loop() {
  // ① 리드스위치: 바뀌는 순간 바로 출력
  bool now = umbrellaPresent();
  if (now != lastPresent) {
    lastPresent = now;
    printState();
  }

  // ② 시리얼 명령
  if (!Serial.available()) { delay(20); return; }
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  if (cmd == "o") {
    Serial.println("열림 (0도) — 이 상태로 실드 V-G 전압을 재보세요");
    if (!servo.attached()) servo.attach(SERVO_PIN, 500, 2400);
    servo.write(ANGLE_OPEN);              // 열어둔 채 유지 (detach 안 함)
  }
  else if (cmd == "c") {
    Serial.println("닫힘 (180도)");
    moveTo(ANGLE_CLOSED);
  }
  else if (cmd == "t") {
    Serial.println("슬롯1 열림");
    moveTo(ANGLE_OPEN);
    delay(5000 - 900);                    // moveTo가 이미 0.9초 씀
    Serial.println("5초후 닫힘");
    moveTo(ANGLE_CLOSED);
    if (!umbrellaPresent()) Serial.println("대여완료 (우산 없음)");
    else                    Serial.println("대여실패 — 우산이 그대로 있어요");
  }
  else if (cmd == "w") {
    Serial.println("왕복 10회 시작 — 도중에 멈추거나 보드가 리셋되면 전원 부족!");
    for (int i = 1; i <= 10; i++) {
      Serial.printf("  %d회\n", i);
      moveTo(ANGLE_OPEN);
      moveTo(ANGLE_CLOSED);
    }
    Serial.println("왕복 10회 완료 — 여기까지 왔으면 전원은 충분합니다");
  }
  else if (cmd == "s") {
    printState();
  }
  else {
    Serial.println("모르는 명령: " + cmd + "  (o/c/t/w/s)");
  }
}
