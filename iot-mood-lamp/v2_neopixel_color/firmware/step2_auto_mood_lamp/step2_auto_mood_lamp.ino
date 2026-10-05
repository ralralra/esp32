/*
  IoT 무드등 V2 · 2단계 — 자동 무드등 + 네오픽셀 (와이파이 없이)

  보드: ESP32 DevKit 30핀 + 확장 베이스보드 (전압 점퍼 5V)
  배선: ../../wiring.md
    네오픽셀 링 → D27 줄 (DIN→S, 5V→V, GND→G) — DIN 선에 330Ω, 링 5V–GND에 1000µF
    PIR         → D25 줄 (OUT→S, VCC→V, GND→G)
    조도센서    → D34 줄 (AO→S, GND→G), VCC만 위쪽 3.3V 헤더
  라이브러리: Adafruit NeoPixel

  동작: V1 3단계와 같은 자동 판단 + 마지막 색·밝기로 '서서히' 켜고 끄기

  시리얼 명령 (115200, 줄바꿈 포함):
    on / off / auto / s(상태)
    sensor           센서 값 1초마다 찍기 켜기/끄기 (조도 원시값·%·어두움 판정, PIR, 자동 꺼짐까지 남은 시간)
                     → 조도 기준값(DARK_ON_LEVEL·DARK_OFF_LEVEL)을 정할 때 이걸 켜 두고 교실 불을 껐다 켜 보세요
    c warm | white | red | orange | yellow | green | blue | purple | pink
    c 255 120 0      (R G B — 0~255)
    b 60             (밝기 0~100 %)
*/

#include <Adafruit_NeoPixel.h>

// ── 핀 ───────────────────────────────────────────
const int LED_PIN   = 27;
const int PIR_PIN   = 25;
const int LIGHT_PIN = 34;

// ── 네오픽셀 설정 ─────────────────────────────────
const int  LED_COUNT      = 24;    // 24구 링
const bool LED_RGBW       = true;   // RGBW 링이면 true, RGB 링이면 false
const int  MAX_BRIGHTNESS = 128;    // 0~255 — 어댑터 보호 상한 (24구 RGBW·5V 2A 기준: 네오픽셀 최대 약 1A)
const unsigned long FADE_MS = 1000; // 켜고 끌 때 걸리는 시간

// ── 자동 판단 설정 (V1과 같음) ──────────────────────
const bool DARK_IS_HIGH   = false;
const int  DARK_ON_LEVEL  = 1200;
const int  DARK_OFF_LEVEL = 1500;
const unsigned long AUTO_OFF_MS   = 5UL * 60 * 1000;
const unsigned long LIGHT_READ_MS = 200;
const unsigned long SENSOR_PRINT_MS = 1000;   // 'sensor' 켰을 때 찍는 간격

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, (LED_RGBW ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);

// ── 상태 ─────────────────────────────────────────
bool lampOn = false;
bool autoMode = true;
bool isDark = false;
int  lightAvg = 0;
unsigned long lastMotionMs = 0;
unsigned long lastLightMs = 0;
unsigned long lastSensorPrintMs = 0;
bool sensorPrint = false;             // 'sensor' 명령으로 켜고 끔
bool lastMotion = false;

uint8_t colR = 0, colG = 0, colB = 0, colW = 255;  // 기본: 웜화이트(W 채널)
String  colName = "warm";
int     brightPct = 60;                             // 0~100

int fadeFrom = 0, fadeTo = 0, level = 0;            // 현재 밝기 0~MAX_BRIGHTNESS
unsigned long fadeStart = 0;
String inputLine;

int targetLevel() { return lampOn ? (long)MAX_BRIGHTNESS * brightPct / 100 : 0; }

void startFade() {
  fadeFrom = level;
  fadeTo = targetLevel();
  fadeStart = millis();
}

void showRing() {
  for (int i = 0; i < LED_COUNT; i++) {
    if (LED_RGBW) ring.setPixelColor(i, colR, colG, colB, colW);
    else          ring.setPixelColor(i, colR, colG, colB);
  }
  ring.setBrightness(level);
  ring.show();
}

// 서서히 밝기를 바꾼다 — delay() 없이 millis()로
void updateFade(unsigned long now) {
  if (level == fadeTo) return;
  unsigned long t = now - fadeStart;
  if (t >= FADE_MS) level = fadeTo;
  else level = fadeFrom + (long)(fadeTo - fadeFrom) * (long)t / (long)FADE_MS;
  showRing();
}

void setLamp(bool on) {
  lampOn = on;
  startFade();
}

bool setColorByName(String name) {
  struct Preset { const char* n; uint8_t r, g, b, w; };
  static const Preset presets[] = {
    {"warm", 0, 0, 0, 255},   {"white", 0, 0, 0, 255}, {"red", 255, 0, 0, 0},
    {"orange", 255, 90, 0, 0}, {"yellow", 255, 200, 0, 0}, {"green", 0, 255, 0, 0},
    {"blue", 0, 60, 255, 0},  {"purple", 150, 0, 255, 0}, {"pink", 255, 60, 120, 0},
  };
  for (const Preset& p : presets) {
    if (name == p.n) {
      colR = p.r; colG = p.g; colB = p.b; colW = p.w;
      if (!LED_RGBW && colW) { colR = 255; colG = 170; colB = 80; }  // RGB 링의 웜화이트 근사
      if (name == "white" && !LED_RGBW) { colR = colG = colB = 255; }
      if (name == "white" && LED_RGBW)  { colR = colG = colB = 120; colW = 255; }
      colName = name;
      return true;
    }
  }
  return false;
}

void updateDarkness(int raw) {
  bool darkNow   = DARK_IS_HIGH ? raw >= DARK_ON_LEVEL  : raw <= DARK_ON_LEVEL;
  bool brightNow = DARK_IS_HIGH ? raw <= DARK_OFF_LEVEL : raw >= DARK_OFF_LEVEL;
  if (!isDark && darkNow)   isDark = true;
  if (isDark  && brightNow) isDark = false;
}

void printStatus() {
  Serial.printf("[상태] 램프=%s 모드=%s 색=%s(%d,%d,%d,%d) 밝기=%d%% 출력=%d/%d 조도=%d 어두움=%s PIR=%d\n",
                lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL",
                colName.c_str(), colR, colG, colB, colW, brightPct, level, MAX_BRIGHTNESS,
                lightAvg, isDark ? "예" : "아니오", digitalRead(PIR_PIN));
}

// 센서 값 한 줄 — 기준값을 정할 때 보는 화면
//   조도: 원시값(0~4095)과 %, 지금 '어두움'으로 보는지, 켜짐/꺼짐 기준값
//   PIR : 0/1, 마지막 감지 후 지난 시간, 자동 꺼짐까지 남은 시간
void printSensors(unsigned long now) {
  int pct = (long)lightAvg * 100 / 4095;
  unsigned long since = (now - lastMotionMs) / 1000;
  char remain[32] = "";
  if (lampOn && autoMode) snprintf(remain, sizeof(remain), " | 자동 꺼짐까지 %lds", (long)(AUTO_OFF_MS / 1000) - (long)since);
  Serial.printf("[센서] 조도=%4d (%3d%%) %s  기준: 켜짐≤%d 꺼짐≥%d | PIR=%d 마지막 감지 %lus 전%s\n",
                lightAvg, pct, isDark ? "어두움" : "밝음  ", DARK_ON_LEVEL, DARK_OFF_LEVEL,
                digitalRead(PIR_PIN), since, remain);
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd == "on")        { autoMode = false; setLamp(true);  Serial.println("수동 켜기"); }
  else if (cmd == "off")  { autoMode = false; setLamp(false); Serial.println("수동 끄기"); }
  else if (cmd == "auto") { autoMode = true;  lastMotionMs = millis(); Serial.println("자동 모드"); }
  else if (cmd == "s")    { printStatus(); }
  else if (cmd == "sensor") {
    sensorPrint = !sensorPrint;
    Serial.println(sensorPrint ? "센서 값 1초마다 표시 — 'sensor'를 다시 입력하면 멈춤" : "센서 값 표시 끔");
    if (sensorPrint) printSensors(millis());
  }
  else if (cmd.startsWith("c ")) {
    String arg = cmd.substring(2);
    arg.trim();
    int r, g, b;
    if (sscanf(arg.c_str(), "%d %d %d", &r, &g, &b) == 3) {
      colR = constrain(r, 0, 255); colG = constrain(g, 0, 255); colB = constrain(b, 0, 255); colW = 0;
      colName = "custom";
    } else if (!setColorByName(arg)) {
      Serial.println("색 이름: warm white red orange yellow green blue purple pink");
      return;
    }
    showRing();
    Serial.printf("색 → %s\n", colName.c_str());
  }
  else if (cmd.startsWith("b ")) {
    brightPct = constrain(cmd.substring(2).toInt(), 0, 100);
    startFade();
    Serial.printf("밝기 → %d%%\n", brightPct);
  }
  else if (cmd.length()) { Serial.println("명령: on / off / auto / s / sensor / c <색> / b <0~100>"); }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { handleCommand(inputLine); inputLine = ""; }
    else inputLine += c;
  }
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  ring.begin();
  ring.clear();
  ring.show();                       // 부팅 직후 모든 LED 끄기
  lightAvg = analogRead(LIGHT_PIN);
  updateDarkness(lightAvg);
  Serial.println("IoT 무드등 V2 — 자동 모드 시작 (PIR는 전원 후 수십 초 안정화 필요)");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;
    updateDarkness(lightAvg);
  }

  bool motion = digitalRead(PIR_PIN) == HIGH;
  if (motion) lastMotionMs = now;
  if (motion != lastMotion) {                 // PIR가 바뀌는 순간은 간격과 상관없이 바로 알림
    lastMotion = motion;
    if (sensorPrint) Serial.println(motion ? "[센서] PIR 감지됨 ▲" : "[센서] PIR 해제 ▽");
  }
  if (sensorPrint && now - lastSensorPrintMs >= SENSOR_PRINT_MS) {
    lastSensorPrintMs = now;
    printSensors(now);
  }

  if (autoMode) {
    if (!lampOn) {
      if (motion && isDark) { setLamp(true); Serial.println("자동 켜기 (어두움 + 사람 감지)"); }
    } else if (now - lastMotionMs >= AUTO_OFF_MS) {
      setLamp(false);
      Serial.println("자동 끄기 (사람 없음)");
    }
  }

  updateFade(now);
}
