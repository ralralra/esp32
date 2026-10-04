/*
  토양습도 센서 — 원시값을 0~100%로 번역 (캘리브레이션)
  연결: 점퍼선 3가닥 — G→GND, V→3V3(반드시 3V3!), S→A2~A5 핀 (그림: images/wiring_analog_sensor.png)

  1단계: 아래 %는 무시하고 raw만 관찰 →
         공기 중(건조) 값과 물컵 속(습윤) 값을 기록
  2단계: DRY/WET에 기록한 값을 넣으면 %가 완성!
*/

#define SOIL 34  // A3 핀 예시 (A2=35 · A4=36 · A5=39)

int DRY = 3100;  // ★ 우리 센서의 공기 중(건조) 값으로 교체
int WET = 1300;  // ★ 우리 센서의 물컵 속(습윤) 값으로 교체
// %가 반대로 움직이면 DRY·WET을 서로 바꾸세요

void setup() {
  Serial.begin(115200);
}

void loop() {
  int raw = analogRead(SOIL);
  int pct = map(raw, DRY, WET, 0, 100);
  pct = constrain(pct, 0, 100);

  Serial.print("raw: ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(pct);
  Serial.println("%");
  delay(500);
}
