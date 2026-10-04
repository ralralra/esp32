/*
  1회차 · 따라하기 ① — 내장 LED 깜빡이기 (첫 업로드 테스트)
  보드: Wemos D1 R32 (툴 → 보드 → ESP32 Dev Module)
  배선: 없음! 보드만 있으면 됩니다. 내장 LED = GPIO2
*/

#define LED 2  // D1 R32 내장 LED

void setup() {            // 처음 1번 실행
  pinMode(LED, OUTPUT);   // 2번핀 = 출력
}

void loop() {              // 계속 반복
  digitalWrite(LED, HIGH); // 켜기
  delay(1000);             // 1초 대기
  digitalWrite(LED, LOW);  // 끄기
  delay(1000);
}
