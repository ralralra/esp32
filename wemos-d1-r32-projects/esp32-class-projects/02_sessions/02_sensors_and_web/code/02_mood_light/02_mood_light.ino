/*
  2회차 · 미션 ① — 빛 반응 무드등 (예시 답안)
  어두워질수록 내장 LED가 밝아져요.
  map()은 '범위 번역기' — 뒤 두 숫자(255, 0)를 바꾸면 방향이 반전!
*/

#define LED   2   // 내장 LED
#define LIGHT 34  // 빛센서 (A3 핀 — A2~A5 핀만, 핀맵 문서 확인)

void setup() {
  Serial.begin(115200);
}

void loop() {
  int l = analogRead(LIGHT);          // 0(어두움) ~ 4095(밝음)
  int b = map(l, 0, 4095, 255, 0);    // 반전: 어두울수록 b가 커짐
  analogWrite(LED, b);                // LED 밝기 조절
  Serial.print("light:");
  Serial.print(l);
  Serial.print(" -> bright:");
  Serial.println(b);
  delay(100);
}
