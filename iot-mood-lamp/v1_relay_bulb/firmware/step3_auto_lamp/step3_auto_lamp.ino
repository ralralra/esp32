/*
  IoT 무드등 V1 · 3단계 — 자동 무드등 (와이파이 없이)

  보드: ESP32 DevKit 30핀 + 확장 베이스보드 (전압 점퍼 5V)
  배선: ../../wiring.md
    릴레이   → D26 줄 (IN→S, VCC→V, GND→G)
    PIR      → D25 줄 (OUT→S, VCC→V, GND→G)
    조도센서 → D34 줄 (AO→S, GND→G), VCC만 위쪽 3.3V 헤더

  동작:
    자동 모드 — 어둡고 사람이 감지되면 켜고, AUTO_OFF_MS 동안 아무도 없으면 끈다.
               켜진 뒤에는 조도를 보지 않는다(램프 자기 빛 때문에 꺼지는 것 방지).
    수동 모드 — 시리얼 명령으로 켜고 끈다. 'auto'로 자동 모드 복귀.

  시리얼 명령 (115200, 줄바꿈 포함):
    on / off / auto / s(상태)
*/

// ── 핀 ───────────────────────────────────────────
const int RELAY_PIN = 26;
const int PIR_PIN   = 25;
const int LIGHT_PIN = 34;   // ADC1 — 와이파이를 켜도 읽힘

// ── 설정 (2단계에서 실측한 값으로 바꾸기) ─────────────
const bool RELAY_ACTIVE_LOW = true;   // LOW에서 켜지는 릴레이면 true
const bool DARK_IS_HIGH     = false;  // 어두울수록 값이 커지는 모듈이면 true
const int  DARK_ON_LEVEL    = 1200;   // 이보다 어두우면 '어둡다'
const int  DARK_OFF_LEVEL   = 1500;   // 이보다 밝아져야 '밝다' (히스테리시스)
const unsigned long AUTO_OFF_MS   = 5UL * 60 * 1000;  // 사람이 없으면 5분 뒤 끄기
const unsigned long LIGHT_READ_MS = 200;              // 조도 읽기 간격

// ── 상태 ─────────────────────────────────────────
bool lampOn = false;
bool autoMode = true;
bool isDark = false;
int  lightAvg = 0;
unsigned long lastMotionMs = 0;
unsigned long lastLightMs = 0;
String inputLine;

void setRelay(bool on) {
  int level = on ? HIGH : LOW;
  if (RELAY_ACTIVE_LOW) level = on ? LOW : HIGH;
  digitalWrite(RELAY_PIN, level);
  lampOn = on;
}

// '어두운가'를 히스테리시스로 판단 — 경계값에서 깜빡이지 않게
//   DARK_IS_HIGH = false : 값 ≤ DARK_ON_LEVEL 이면 어두움, 값 ≥ DARK_OFF_LEVEL 이면 밝음 (ON < OFF)
//   DARK_IS_HIGH = true  : 값 ≥ DARK_ON_LEVEL 이면 어두움, 값 ≤ DARK_OFF_LEVEL 이면 밝음 (ON > OFF)
void updateDarkness(int raw) {
  bool darkNow   = DARK_IS_HIGH ? raw >= DARK_ON_LEVEL  : raw <= DARK_ON_LEVEL;
  bool brightNow = DARK_IS_HIGH ? raw <= DARK_OFF_LEVEL : raw >= DARK_OFF_LEVEL;
  if (!isDark && darkNow)   isDark = true;
  if (isDark  && brightNow) isDark = false;
}

void printStatus() {
  Serial.printf("[상태] 램프=%s 모드=%s 조도=%d 어두움=%s PIR=%d\n",
                lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL",
                lightAvg, isDark ? "예" : "아니오", digitalRead(PIR_PIN));
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd == "on")        { autoMode = false; setRelay(true);  Serial.println("수동 켜기"); }
  else if (cmd == "off")  { autoMode = false; setRelay(false); Serial.println("수동 끄기"); }
  else if (cmd == "auto") { autoMode = true;  lastMotionMs = millis(); Serial.println("자동 모드"); }
  else if (cmd == "s")    { printStatus(); }
  else if (cmd.length())  { Serial.println("명령: on / off / auto / s"); }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { handleCommand(inputLine); inputLine = ""; }
    else inputLine += c;
  }
}

void setup() {
  // 부팅 순간 전구가 깜빡이지 않도록 '꺼짐' 값을 먼저 써 두고 출력으로 전환
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  lightAvg = analogRead(LIGHT_PIN);
  updateDarkness(lightAvg);
  Serial.println("IoT 무드등 V1 — 자동 모드 시작 (PIR는 전원 후 수십 초 안정화 필요)");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  // 조도: 일정 간격으로 읽어 이동평균
  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;
    updateDarkness(lightAvg);
  }

  bool motion = digitalRead(PIR_PIN) == HIGH;
  if (motion) lastMotionMs = now;

  if (!autoMode) return;

  if (!lampOn) {
    if (motion && isDark) { setRelay(true); Serial.println("자동 켜기 (어두움 + 사람 감지)"); }
  } else {
    // 켜진 뒤에는 조도를 보지 않는다 — 램프 자기 빛에 꺼지는 것 방지
    if (now - lastMotionMs >= AUTO_OFF_MS) { setRelay(false); Serial.println("자동 끄기 (사람 없음)"); }
  }
}
