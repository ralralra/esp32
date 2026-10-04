/*
  1회차 · 미션 ① — 우리 팀 LED 신호 만들기 (예시 답안)
  대기 = 천천히 3번 / 작업중 = 빠르게 5번 / 완료 = 길게 1번
  delay 값(밀리초)을 바꿔 팀만의 패턴을 만들어 보세요.
*/

#define LED 2  // D1 R32 내장 LED

void blinkTimes(int times, int ms) {  // '내 함수' 만들기 — 재사용!
  for (int i = 0; i < times; i++) {
    digitalWrite(LED, HIGH);
    delay(ms);
    digitalWrite(LED, LOW);
    delay(ms);
  }
}

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  blinkTimes(3, 600);   // 대기: 천천히 3번
  delay(2000);
  blinkTimes(5, 150);   // 작업중: 빠르게 5번
  delay(2000);
  blinkTimes(1, 1500);  // 완료: 길게 1번
  delay(2000);
}
