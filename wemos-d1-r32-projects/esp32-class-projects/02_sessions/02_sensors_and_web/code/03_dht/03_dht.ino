/*
  2회차 · 따라하기 — 온습도 센서(DHT): 교실의 공기를 숫자로
  라이브러리: DHT sensor library (1회차에 설치)
  ⚠ DHT11은 2초에 1번만 측정 가능 — delay(2000) 필수!
  값이 nan이면: 배선·핀번호·측정 간격 확인
*/

#include <DHT.h>

#define DHTPIN 19   // D12 핀 (★꽂은 핀의 GPIO로 — 핀맵 문서 확인)
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();  // 온도 (℃)
  float h = dht.readHumidity();     // 습도 (%)
  Serial.print(t);
  Serial.print("C ");
  Serial.print(h);
  Serial.println("%");
  delay(2000);  // 2초 이상 간격!
}
