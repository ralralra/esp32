/*
  IoT 무드등 V3 · 3단계 — 네오픽셀 + 릴레이 전구 동시 제어 (조도·PIR → 두 조명, 와이파이 없이)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md) · SEL 점퍼 꽂음 · 보드 DC 잭에 9V 건전지
    네오픽셀 스트립 → 실드 6번 (DIN→S, 5V→V, GND→G) = GPIO27
    릴레이(KY-019형, HIGH 트리거) → 실드 2번 (S→S, +→V, −→G) = GPIO26 · 전구는 COM–NO
    PIR          → 실드 3번                          = GPIO25
    조도센서     → 실드 A3 (VCC는 Bluetooth 헤더 3V3) = GPIO34
  라이브러리: Adafruit NeoPixel (NEO_GRB + NEO_KHZ800) · V3 링은 12구 (V1 스트립은 16구)

  규칙 (자동 모드) — 두 조명이 서로 다른 센서를 본다
    네오픽셀: 사람이 감지되면 켜짐 → 움직임 없이 NP_HOLD_MS 지나면 서서히 꺼짐        (조도와 무관)
    전구    : 주변 밝기 < DARK_LEVEL 이면 켜짐 → DARK_LEVEL + DARK_MARGIN + BULB_GLOW 이상이면 꺼짐 (사람과 무관)
    1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 아래에 똑같이 적는다.

  시리얼(115200) 명령
    on / off            둘 다 켜기·끄기 (수동 모드로)
    np on / np off      네오픽셀만          bulb on / bulb off   전구만
    auto                자동 모드로
    s                   상태 한 줄          sensor               센서 값 1초마다 표시 켜기/끄기
    c <색> / b <0~100>  네오픽셀 색 (warm white red orange yellow green blue purple pink rainbow FF8800) / 밝기 %
*/

#include <Adafruit_NeoPixel.h>

// ── 핀 ───────────────────────────────────────────
const int LED_PIN   = 27;   // 실드 6번
const int RELAY_PIN = 26;   // 실드 2번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3

// ── 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움' → 전구 켜짐
const int  DARK_MARGIN  = 5;      // 기준 + 5% 이상이면 '밝음' → 전구 꺼짐 (경계에서 딸깍거림 방지)
const bool DARK_IS_HIGH = true;   // 어두울수록 값이 커지는 모듈(이번 부품) = true · 가렸는데 %가 내려가면 false
const int  BULB_GLOW    = 0;      // 전구를 켜면 조도센서가 더 밝게 읽는 만큼(%) — 1단계 기록표 '램프 켬' − '불 끔'. 센서가 전구 빛을 보면 여기 적는다
const unsigned long BULB_MIN_ON_MS = 30UL * 1000;   // 전구가 한 번 켜지면 최소 이 시간은 유지 (자기 빛 때문에 바로 꺼지는 것 방지)

// ── 사람 감지 → 네오픽셀 ───────────────────────────
const unsigned long NP_HOLD_MS = 60UL * 1000;      // 움직임이 없으면 이 시간 뒤 네오픽셀 끔
const bool NP_ONLY_WHEN_DARK   = false;            // true면 어두울 때만 사람에 반응

// ── 네오픽셀 ─────────────────────────────────────
#define LED_COUNT 12
const int  MAX_BRIGHTNESS = 64;    // 전원 A안(9V → 보드 레귤레이터)용 상한 (12구 약 0.11A). B·C안이나 5V 2A 충전기면 128까지
const unsigned long FADE_MS = 1000;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ── 릴레이 ───────────────────────────────────────
const bool RELAY_ACTIVE_LOW = false;  // KY-019형(HIGH에서 켜짐) = false · 모듈 LED까지 반대로 켜지는 모듈이면 true (전구만 반대면 NC→NO)

const unsigned long LIGHT_READ_MS   = 200;
const unsigned long SENSOR_PRINT_MS = 1000;

// ── 상태 ─────────────────────────────────────────
bool npOn = false, bulbOn = false, autoMode = true, isDark = false;
bool motion = false, sensorPrint = false;
int  lightAvg = 0, lightPct = 0;
unsigned long lastMotionMs = 0, lastLightMs = 0, lastSensorPrintMs = 0, bulbOnSince = 0;

uint8_t colR = 255, colG = 170, colB = 80;          // 기본: 웜화이트
String  colName = "warm";
bool    rainbow = false;
int     brightPct = 60;
int     fadeFrom = 0, fadeTo = 0, level = 0;
unsigned long fadeStart = 0, lastRainbowMs = 0;
long rainbowHue = 0;
String inputLine;

// ─────────────────────────────────────────────────
// 센서
// ─────────────────────────────────────────────────
int lightPercent(int raw) {
  int pct = ((long)raw * 100 + 2047) / 4095;       // 반올림 — 평균이 끝값에 몇 단위 못 미쳐도 0%·100%
  return DARK_IS_HIGH ? 100 - pct : pct;          // 100 = 밝음
}

void updateDarkness() {
  int offLevel = min(100, DARK_LEVEL + DARK_MARGIN + (bulbOn ? BULB_GLOW : 0));   // 전구가 켜져 있으면 그 빛만큼 더 밝아야 '밝음' (상한 100)
  if (!isDark && lightPct < DARK_LEVEL) isDark = true;
  if (isDark  && lightPct >= offLevel)  isDark = false;
}

// ─────────────────────────────────────────────────
// 네오픽셀
// ─────────────────────────────────────────────────
int  targetLevel() { return npOn ? (long)MAX_BRIGHTNESS * brightPct / 100 : 0; }
void startFade()   { fadeFrom = level; fadeTo = targetLevel(); fadeStart = millis(); }
void setNp(bool on) { npOn = on; startFade(); }

void showStrip() {
  if (rainbow) strip.rainbow(rainbowHue);
  else for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, colR, colG, colB);
  strip.setBrightness(level);
  strip.show();
}

void updateLights(unsigned long now) {
  bool dirty = false;
  if (level != fadeTo) {
    unsigned long t = now - fadeStart;
    level = t >= FADE_MS ? fadeTo : fadeFrom + (long)(fadeTo - fadeFrom) * (long)t / (long)FADE_MS;
    dirty = true;
  }
  if (rainbow && level > 0 && now - lastRainbowMs >= 10) {
    lastRainbowMs = now; rainbowHue += 256; if (rainbowHue >= 5 * 65536L) rainbowHue = 0; dirty = true;
  }
  if (dirty) showStrip();
}

bool setColor(String v) {
  struct Preset { const char* n; uint8_t r, g, b; };
  static const Preset presets[] = {
    {"warm", 255, 170, 80}, {"white", 255, 255, 255}, {"red", 255, 0, 0},
    {"orange", 255, 90, 0}, {"yellow", 255, 200, 0},  {"green", 0, 255, 0},
    {"blue", 0, 60, 255},   {"purple", 150, 0, 255},  {"pink", 255, 60, 120},
  };
  v.trim(); v.toLowerCase();
  if (v == "rainbow") { rainbow = true; colName = "rainbow"; return true; }
  for (const Preset& p : presets)
    if (v == p.n) { colR = p.r; colG = p.g; colB = p.b; rainbow = false; colName = p.n; return true; }
  if (v.length() == 6) {
    for (unsigned i = 0; i < 6; i++) if (!isxdigit((unsigned char)v[i])) return false;
    long rgb = strtol(v.c_str(), nullptr, 16);
    colR = (rgb >> 16) & 0xFF; colG = (rgb >> 8) & 0xFF; colB = rgb & 0xFF;
    rainbow = false; v.toUpperCase(); colName = v;
    return true;
  }
  return false;
}

// ─────────────────────────────────────────────────
// 전구
// ─────────────────────────────────────────────────
void setBulb(bool on) {
  if (on && !bulbOn) bulbOnSince = millis();
  bulbOn = on;
  digitalWrite(RELAY_PIN, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

// ─────────────────────────────────────────────────
// 시리얼
// ─────────────────────────────────────────────────
void printStatus() {
  Serial.printf("[상태] 네오픽셀=%s(%s %d%%) 전구=%s 모드=%s | 주변=%d%% %s(기준 %d%%) PIR=%d\n",
                npOn ? "ON" : "OFF", colName.c_str(), brightPct, bulbOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL",
                lightPct, isDark ? "어두움" : "밝음", DARK_LEVEL, motion);
}

void printSensors(unsigned long now) {
  unsigned long since = (now - lastMotionMs) / 1000;
  char remain[64] = "";
  if (npOn && autoMode) {
    long left = (long)(NP_HOLD_MS / 1000) - (long)since;
    snprintf(remain, sizeof(remain), " | 움직임 없으면 %lds 뒤 네오픽셀 꺼짐", left < 0 ? 0 : left);
  }
  Serial.printf("[센서] 조도=%4d 밝기=%3d%% → %s (기준 %d%%) | PIR=%d 마지막 감지 %lus 전%s\n",
                lightAvg, lightPct, isDark ? "어두움" : "밝음  ", DARK_LEVEL, motion, since, remain);
}

void handleCommand(String line) {
  line.trim();
  String lower = line; lower.toLowerCase();
  if (lower == "on")             { autoMode = false; setNp(true);  setBulb(true);  Serial.println("수동: 둘 다 켜기"); }
  else if (lower == "off")       { autoMode = false; setNp(false); setBulb(false); Serial.println("수동: 둘 다 끄기"); }
  else if (lower == "np on")     { autoMode = false; setNp(true);    Serial.println("수동: 네오픽셀 켜기"); }
  else if (lower == "np off")    { autoMode = false; setNp(false);   Serial.println("수동: 네오픽셀 끄기"); }
  else if (lower == "bulb on")   { autoMode = false; setBulb(true);  Serial.println("수동: 전구 켜기"); }
  else if (lower == "bulb off")  { autoMode = false; setBulb(false); Serial.println("수동: 전구 끄기"); }
  else if (lower == "auto")      { autoMode = true;  lastMotionMs = millis(); Serial.println("자동 모드 — 사람→네오픽셀 · 어두움→전구"); }
  else if (lower == "s")         printStatus();
  else if (lower == "sensor") {
    sensorPrint = !sensorPrint;
    Serial.println(sensorPrint ? "센서 값 1초마다 표시 — 'sensor'를 다시 입력하면 멈춤" : "센서 값 표시 끔");
    if (sensorPrint) printSensors(millis());
  }
  else if (lower.startsWith("c ")) {
    if (!setColor(line.substring(2))) { Serial.println("색: warm white red orange yellow green blue purple pink rainbow 또는 FF8800"); return; }
    showStrip();
    Serial.println("색 → " + colName);
  }
  else if (lower.startsWith("b ")) { brightPct = constrain(lower.substring(2).toInt(), 0, 100); startFade(); Serial.printf("밝기 → %d%%\n", brightPct); }
  else if (line.length()) Serial.println("명령: on / off / np on|off / bulb on|off / auto / s / sensor / c <색> / b <0~100>");
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
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 전구 깜빡임 방지: '꺼짐' 먼저
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();
  lightAvg = analogRead(LIGHT_PIN);
  lightPct = lightPercent(lightAvg);
  updateDarkness();
  Serial.printf("IoT 무드등 V3 (네오픽셀 + 전구) — 자동 모드 · 사람→네오픽셀(%lus 유지) · 어두움(%d%%)→전구\n", NP_HOLD_MS / 1000, DARK_LEVEL);
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
    // 네오픽셀 ← 사람
    bool npAllowed = !NP_ONLY_WHEN_DARK || isDark;
    if (!npOn && motion && npAllowed)                       { setNp(true);  Serial.println("자동: 네오픽셀 켜기 (사람)"); }
    else if (npOn && now - lastMotionMs >= NP_HOLD_MS)      { setNp(false); Serial.println("자동: 네오픽셀 끄기 (사람 없음)"); }
    else if (npOn && NP_ONLY_WHEN_DARK && !isDark)          { setNp(false); Serial.println("자동: 네오픽셀 끄기 (밝아짐)"); }

    // 전구 ← 어두움
    if (!bulbOn && isDark)                                                    { setBulb(true);  Serial.println("자동: 전구 켜기 (어두움)"); }
    else if (bulbOn && !isDark && now - bulbOnSince >= BULB_MIN_ON_MS)        { setBulb(false); Serial.println("자동: 전구 끄기 (밝아짐)"); }
  }
  updateLights(now);
}
