/*
  3단계 — 자동 급수 (릴레이 + 펌프)
  ----------------------------------------------------
  목표: 흙이 마르면 릴레이로 펌프를 켜서 잠깐 물을 주고,
        물이 스며들 시간을 기다린 뒤 다시 측정한다.

  ⚠ 펌프 전원은 반드시 외부 5V! ESP32는 릴레이로 스위치만 한다.
  배선: 릴레이 IN → GPIO26 (펌프는 릴레이를 통해 외부전원에 연결)
*/

#define SOIL_PIN  34
#define RELAY_PIN 26

int  DRY_THRESHOLD = 3000;   // ★ 내 센서 기준값
int  WATER_MS      = 2000;   // 한 번에 급수할 시간(2초)
int  SOAK_MS       = 5000;   // 급수 후 물이 스며들 대기 시간(5초)

// 릴레이가 active-LOW 라고 가정 (LOW = 펌프 ON)
// 펌프가 반대로 동작하면 ON/OFF 값을 서로 바꾸세요.
const int PUMP_ON  = LOW;
const int PUMP_OFF = HIGH;

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, PUMP_OFF);   // 시작은 펌프 꺼짐
  Serial.println("\n=== 자동 급수 ===");
  Serial.printf("마름 기준 = %d, 급수 %dms, 대기 %dms\n",
                DRY_THRESHOLD, WATER_MS, SOAK_MS);
}

void loop() {
  int raw = analogRead(SOIL_PIN);
  Serial.printf("수분 값: %4d → ", raw);

  if (raw > DRY_THRESHOLD) {
    Serial.println("말랐음! 💦 급수 시작");
    digitalWrite(RELAY_PIN, PUMP_ON);    // 펌프 ON
    delay(WATER_MS);                      // 잠깐 물 주기
    digitalWrite(RELAY_PIN, PUMP_OFF);   // 펌프 OFF
    Serial.println("급수 완료, 스며들 시간 대기...");
    delay(SOAK_MS);                       // 물이 퍼질 때까지 기다림
  } else {
    Serial.println("촉촉함 👍 (급수 안 함)");
    delay(1000);
  }
}
