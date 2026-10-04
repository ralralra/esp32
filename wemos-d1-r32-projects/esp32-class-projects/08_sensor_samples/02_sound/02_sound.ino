/*
  소리 센서 — 시끄러운 '정도' 읽기
  연결: 점퍼선 3가닥 — G→GND, V→3V3(반드시 3V3!), S→A2~A5 핀 (그림: images/wiring_analog_sensor.png)
  ★ 연결한 핀의 GPIO로 수정: A2=35 · A3=34 · A4=36 · A5=39
*/

#define SOUND 35  // A2 핀 예시
#define LOUD  2500  // '시끄러움' 기준값 — 교실에 맞게 조절!

void setup() {
  Serial.begin(115200);
}

void loop() {
  int v = analogRead(SOUND);  // 0 ~ 4095
  Serial.print("sound: ");
  Serial.print(v);
  if (v > LOUD) Serial.print("  <- 시끄러워요!");
  Serial.println();
  delay(200);  // 박수를 쳐 보세요!
}
