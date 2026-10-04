/*
  2단계 — 마름/촉촉 판단하기
  ----------------------------------------------------
  목표: 1단계에서 정한 "마름 기준(임계값)"으로 ESP32가 스스로
        "흙이 말랐어요 / 촉촉해요" 를 판단해서 알려준다.

  아직 펌프는 없음. 판단 로직만 익힌다.
*/

#define SOIL_PIN 34

// ★ 1단계에서 적어둔 내 센서의 "마름 기준"으로 바꾸세요!
int DRY_THRESHOLD = 3000;   // 이 값보다 크면 = 말랐다

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== 마름/촉촉 판단 ===");
  Serial.printf("마름 기준(임계값) = %d\n", DRY_THRESHOLD);
}

void loop() {
  int raw = analogRead(SOIL_PIN);

  if (raw > DRY_THRESHOLD) {
    Serial.printf("%4d → 🏜  흙이 말랐어요! 물이 필요해요\n", raw);
  } else {
    Serial.printf("%4d → 💧 촉촉해요. 괜찮아요\n", raw);
  }

  delay(1000);
}
