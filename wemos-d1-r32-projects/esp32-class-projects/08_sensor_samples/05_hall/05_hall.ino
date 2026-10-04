/*
  홀(자석) 센서 — 문 열림 감지 (출석·안전 프로젝트의 핵심!)
  연결: 점퍼선 3가닥 — G→GND, V→3V3, S→디지털 핀 아무 곳 (그림: images/wiring_digital_sensor.png)
  사용법: 센서 옆에 자석을 가까이/멀리 — 문틀에 센서, 문에 자석을 붙이면 도어 센서!
*/

#define HALL 27  // D6 핀 예시 (D8→12 · D7→14 · D6→27 · D5→16)
#define LED  2

void setup() {
  Serial.begin(115200);
  pinMode(HALL, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  int v = digitalRead(HALL);  // 자석 감지 시 값이 바뀜 (센서에 따라 0 또는 1)

  if (v == 1) {               // ★ 우리 센서 기준으로 조건 확인 후 조정
    Serial.println("문 열림!  (자석 멀어짐)");
    digitalWrite(LED, HIGH);
  } else {
    Serial.println("문 닫힘   (자석 감지)");
    digitalWrite(LED, LOW);
  }
  delay(200);
}
