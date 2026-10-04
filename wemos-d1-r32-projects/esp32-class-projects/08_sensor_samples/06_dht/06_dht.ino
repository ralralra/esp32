/*
  온습도(DHT) 센서 — 교실의 공기를 숫자로
  연결: 점퍼선 3가닥 — G→GND, V→3V3, S→디지털 핀 아무 곳 (그림: images/wiring_digital_sensor.png)
  라이브러리: DHT sensor library (Adafruit)
  ⚠ DHT11은 2초에 1번만 측정 — delay(2000) 필수! 값이 nan이면 배선·핀·간격 확인
*/

#include <DHT.h>

#define DHTPIN 27  // D6 핀 예시 (D8→12 · D7→14 · D6→27 · D5→16)
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();  // ℃
  float h = dht.readHumidity();     // %
  Serial.print(t);
  Serial.print("C  ");
  Serial.print(h);
  Serial.println("%");
  delay(2000);  // 센서에 입김을 불어 보세요 — 습도 급상승!
}
