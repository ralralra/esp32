/*
  IoT 무드등 V2 · 3단계 — 센서 받고 출력하기 (조도·PIR → 릴레이 전구, 와이파이 없이)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    릴레이(KY-019형, HIGH 트리거) → 실드 2번 (S→S, +→V, −→G) = GPIO26
    PIR                          → 실드 3번                  = GPIO25
    조도센서                     → 실드 A3 (VCC는 Bluetooth 헤더 3V3) = GPIO34

  규칙 (자동 모드)
    켜짐: 주변 밝기가 DARK_LEVEL 보다 어두움 + (USE_PIR이면) 움직임 감지
    꺼짐: 기준보다 밝아짐  또는  (USE_PIR이면) 움직임 없이 AUTO_OFF_MS 지남
    1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 아래에 똑같이 적는다.

  시리얼(115200) 명령
    on / off / auto     수동 켜기·끄기 / 자동 모드로
    s                   상태 한 줄
    sensor              센서 값 1초마다 표시 켜기/끄기
*/

// ── 핀 ───────────────────────────────────────────
const int RELAY_PIN = 26;   // 실드 2번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3

// ── 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움' → 켜짐
const int  DARK_MARGIN  = 5;      // 기준 + 5% 이상이면 '밝음' → 꺼짐 (경계에서 깜빡임 방지)
const bool DARK_IS_HIGH = true;   // 어두울수록 값이 커지는 모듈(이번 부품) = true · 가렸는데 %가 내려가면 false
const bool USE_PIR      = true;   // false면 PIR 없이 조도만으로 켜고 끔
const unsigned long AUTO_OFF_MS = 5UL * 60 * 1000;   // 움직임이 없으면 이 시간 뒤 끔 (USE_PIR일 때)

// ── 릴레이 ───────────────────────────────────────
const bool RELAY_ACTIVE_LOW = false;  // KY-019형(HIGH에서 켜짐) = false

const unsigned long LIGHT_READ_MS   = 200;
const unsigned long SENSOR_PRINT_MS = 1000;

// ── 상태 ─────────────────────────────────────────
bool lampOn = false, autoMode = true, isDark = false;
bool motion = false, sensorPrint = false;
int  lightAvg = 0, lightPct = 0;
unsigned long lastMotionMs = 0, lastLightMs = 0, lastSensorPrintMs = 0;
String inputLine;

// ─────────────────────────────────────────────────
int lightPercent(int raw) {
  int pct = (long)raw * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;          // 100 = 밝음
}

void updateDarkness() {
  if (!isDark && lightPct < DARK_LEVEL)                isDark = true;
  if (isDark  && lightPct >= DARK_LEVEL + DARK_MARGIN) isDark = false;
}

void setLamp(bool on) {
  lampOn = on;
  digitalWrite(RELAY_PIN, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void printStatus() {
  Serial.printf("[상태] 전구=%s 모드=%s | 주변=%d%% %s(기준 %d%%) PIR=%d\n",
                lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL", lightPct, isDark ? "어두움" : "밝음", DARK_LEVEL, motion);
}

void printSensors(unsigned long now) {
  unsigned long since = (now - lastMotionMs) / 1000;
  char remain[40] = "";
  if (lampOn && autoMode && USE_PIR)
    snprintf(remain, sizeof(remain), " | 움직임 없으면 %lds 뒤 꺼짐", (long)(AUTO_OFF_MS / 1000) - (long)since);
  Serial.printf("[센서] 조도=%4d 밝기=%3d%% → %s (기준 %d%%) | PIR=%d 마지막 감지 %lus 전%s\n",
                lightAvg, lightPct, isDark ? "어두움" : "밝음  ", DARK_LEVEL, motion, since, remain);
}

void handleCommand(String line) {
  line.trim();
  line.toLowerCase();
  if (line == "on")        { autoMode = false; setLamp(true);  Serial.println("수동 켜기"); }
  else if (line == "off")  { autoMode = false; setLamp(false); Serial.println("수동 끄기"); }
  else if (line == "auto") { autoMode = true;  lastMotionMs = millis(); Serial.println("자동 모드"); }
  else if (line == "s")    printStatus();
  else if (line == "sensor") {
    sensorPrint = !sensorPrint;
    Serial.println(sensorPrint ? "센서 값 1초마다 표시 — 'sensor'를 다시 입력하면 멈춤" : "센서 값 표시 끔");
    if (sensorPrint) printSensors(millis());
  }
  else if (line.length()) Serial.println("명령: on / off / auto / s / sensor");
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { handleCommand(inputLine); inputLine = ""; }
    else inputLine += c;
  }
}

// ─────────────────────────────────────────────────
void setup() {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 깜빡임 방지: '꺼짐' 먼저
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  lightAvg = analogRead(LIGHT_PIN);
  lightPct = lightPercent(lightAvg);
  updateDarkness();
  Serial.printf("IoT 무드등 V2 (릴레이 전구) — 자동 모드 · 어두움 기준 %d%% · PIR %s\n", DARK_LEVEL, USE_PIR ? "사용" : "사용 안 함");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;
    lightPct = lightPercent(lightAvg);
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; if (sensorPrint) Serial.println(m ? "[센서] PIR ▲ 감지됨" : "[센서] PIR ▽ 해제"); }
  if (sensorPrint && now - lastSensorPrintMs >= SENSOR_PRINT_MS) { lastSensorPrintMs = now; printSensors(now); }

  if (autoMode) {
    bool present = !USE_PIR || motion;
    if (!lampOn && isDark && present)                                  { setLamp(true);  Serial.println(USE_PIR ? "자동 켜기 (어두움 + 사람)" : "자동 켜기 (어두움)"); }
    else if (lampOn && !isDark)                                        { setLamp(false); Serial.println("자동 끄기 (밝아짐)"); }
    else if (lampOn && USE_PIR && now - lastMotionMs >= AUTO_OFF_MS)   { setLamp(false); Serial.println("자동 끄기 (사람 없음)"); }
  }
}
