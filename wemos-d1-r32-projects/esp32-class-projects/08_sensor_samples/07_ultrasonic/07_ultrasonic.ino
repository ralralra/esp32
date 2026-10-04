/*
  초음파 센서 — 박쥐처럼 거리 재기
  연결: 점퍼선 4가닥 (그림: images/wiring_ultrasonic.png)
       GND→GND, VCC→전원, TRIG→D6 핀(GPIO27), ECHO→D7 핀(GPIO14)
  ⚠ 5V 전용 모듈(HC-SR04 등)은 VCC→5V, ECHO 출력이 5V라서
    분압 저항(1kΩ 직렬 + 2kΩ to GND)을 거쳐 D7 핀에 연결! (3.3V 동작 모듈이면 3V3에 바로)
  ⚠ 항상 0이 나오면 TRIG·ECHO가 바뀐 것!
*/

#define TRIG 27  // D6 핀
#define ECHO 14  // D7 핀

long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(ECHO, HIGH);  // 왕복 시간
  return t * 0.034 / 2;          // cm (소리 초속 340m, 왕복 ÷2)
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  Serial.print(readDistance());
  Serial.println(" cm");
  delay(300);  // 자로 실측해서 비교해 보세요!
}
