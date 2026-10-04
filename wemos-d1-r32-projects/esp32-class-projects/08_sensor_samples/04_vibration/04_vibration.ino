/*
  진동 센서 — 충격·움직임 감지 (디지털 0/1)
  연결: 점퍼선 3가닥 — G→GND, V→3V3, S→디지털 핀 아무 곳 (그림: images/wiring_digital_sensor.png)
  ★ 보드 라벨→GPIO: D8→12 · D7→14 · D6→27 · D5→16
*/

#define VIB 27  // D6 핀 예시
#define LED 2   // 내장 LED — 충격 오면 켜기

void setup() {
  Serial.begin(115200);
  pinMode(VIB, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  int v = digitalRead(VIB);  // 0 또는 1
  digitalWrite(LED, v);      // 충격 순간 LED 반짝
  Serial.print("vib: ");
  Serial.println(v);
  delay(100);  // 책상을 톡톡 두드려 보세요!
}
