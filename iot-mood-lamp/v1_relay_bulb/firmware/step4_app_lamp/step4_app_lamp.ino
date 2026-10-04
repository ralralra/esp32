/*
  IoT 무드등 V1 · 4단계 — 앱·음성 연동 완성 펌웨어 (릴레이 전구형)

  ① 아래 "여기만 바꾸기" 4줄을 고친 뒤 업로드하세요.
     WIFI_SSID / WIFI_PASS  : 2.4GHz 와이파이 (5GHz는 ESP32가 못 붙어요)
     SERVER_URL             : 선생님이 배포한 Apps Script 웹앱 주소 (끝이 /exec)
     TEAM_ID                : 내 팀 번호 TEAM01 ~ TEAM12 (반 전체가 시트 하나를 같이 써서 꼭 달라야 해요)

  보드: ESP32 DevKit + 베이스보드 / Wemos D1 R32 (+센서쉴드) — 핀 번호 같음
    릴레이   → GPIO26   PIR → GPIO25   조도센서 → GPIO34 (VCC는 3.3V)

  동작
    - 2초마다 서버에서 내 팀 명령을 하나씩 가져와 실행 (ON · OFF · AUTO · CONFIG)
    - 상태가 바뀌면 바로, 아니어도 30초마다 서버에 보고 → 앱 화면에 표시
    - 통신은 다른 코어에서 돌아서, 와이파이가 느리거나 끊겨도 자동 켜기·끄기는 계속 동작
    - CONFIG(어두움 기준·자동 꺼짐 시간)는 보드에 저장되어 껐다 켜도 유지

  시리얼(115200) 명령: on / off / auto / s(상태) / n(서버 연결 확인)
  라이브러리: 추가 설치 없음 (ESP32 보드 패키지에 포함)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>

// ── ① 여기만 바꾸기 ─────────────────────────────────
const char* WIFI_SSID  = "와이파이이름";
const char* WIFI_PASS  = "와이파이비밀번호";
const char* SERVER_URL = "https://script.google.com/macros/s/배포ID/exec";
const char* TEAM_ID    = "TEAM01";

// ── 핀 ───────────────────────────────────────────
const int RELAY_PIN = 26;
const int PIR_PIN   = 25;
const int LIGHT_PIN = 34;   // ADC1 — 와이파이를 켜도 읽힘

// ── 부품에 맞춰 바꾸는 설정 ──────────────────────────
const bool RELAY_ACTIVE_LOW = true;   // LOW에서 켜지는 릴레이면 true
const bool DARK_IS_HIGH     = false;  // 어두울수록 값이 커지는 조도 모듈이면 true
const int  DARK_HYST        = 8;      // 어두움 기준 + 8%가 되어야 '밝음'으로 (경계에서 깜빡임 방지)

// ── 통신 간격 ────────────────────────────────────────
const unsigned long POLL_MS        = 2000;    // 명령 확인
const unsigned long HEARTBEAT_MS   = 30000;   // 바뀐 게 없어도 보고
const unsigned long SOFT_REPORT_MS = 5000;    // 움직임 변화 보고 최소 간격
const unsigned long HTTP_TIMEOUT_MS = 8000;
const unsigned long WIFI_RETRY_MS  = 10000;
const unsigned long LIGHT_READ_MS  = 200;

// ── 램프 상태 (loop 전용) ─────────────────────────────
bool lampOn = false;
bool autoMode = true;
bool isDark = false;
bool motion = false;
int  lightPct = 0;                    // 주변 밝기 0~100% (클수록 밝음)
int  darkPct = 30;                    // 이 % 이하면 '어둡다' — 앱 CONFIG로 바뀜
unsigned long autoOffMs = 300000;     // 사람이 없으면 끄기까지 — 앱 CONFIG로 바뀜
unsigned long lastMotionMs = 0;
unsigned long lastLightMs = 0;
String inputLine;
Preferences prefs;

// ── 두 작업(loop ↔ 통신)이 주고받는 것 ─────────────────
struct Command { char cmd[10]; char value[20]; int dark; int off; };
struct Snapshot { bool lampOn, autoMode, motion; int lightPct, darkPct, offSec; };
QueueHandle_t cmdQueue;
Snapshot shared;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool reportNow = true;       // 켜짐·모드·설정이 바뀜 → 바로 보고
volatile bool reportSoon = false;     // 움직임이 바뀜 → 5초에 한 번까지만 보고 (12팀 서버 부담 줄이기)
volatile bool pingNow = false;
volatile int  netLastCode = 0;

// ─────────────────────────────────────────────────
// 램프
// ─────────────────────────────────────────────────
void setRelay(bool on) {
  int level = on ? HIGH : LOW;
  if (RELAY_ACTIVE_LOW) level = on ? LOW : HIGH;
  digitalWrite(RELAY_PIN, level);
  lampOn = on;
}

int readLightPct() {
  int pct = (long)analogRead(LIGHT_PIN) * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;
}

void updateDarkness() {
  if (!isDark && lightPct <= darkPct) isDark = true;
  if (isDark && lightPct >= darkPct + DARK_HYST) isDark = false;
}

void publish() {                         // 통신 작업이 읽어 갈 상태를 한 번에 복사
  Snapshot s = { lampOn, autoMode, motion, lightPct, darkPct, (int)(autoOffMs / 1000) };
  portENTER_CRITICAL(&sharedMux);
  shared = s;
  portEXIT_CRITICAL(&sharedMux);
}

void changed() { publish(); reportNow = true; }

void printStatus() {
  Serial.printf("[%s] 램프=%s 모드=%s 밝기=%d%% 어두움=%s(기준 %d%%) PIR=%d 자동꺼짐=%lus 서버=%d\n",
                TEAM_ID, lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL", lightPct,
                isDark ? "예" : "아니오", darkPct, motion, autoOffMs / 1000, netLastCode);
}

// 서버·시리얼 명령을 같은 곳에서 처리
void applyCommand(const Command& c) {
  String cmd = c.cmd;
  if (cmd == "ON")        { autoMode = false; setRelay(true);  Serial.println("명령: 켜기 (수동)"); }
  else if (cmd == "OFF")  { autoMode = false; setRelay(false); Serial.println("명령: 끄기 (수동)"); }
  else if (cmd == "AUTO") { autoMode = true; lastMotionMs = millis(); Serial.println("명령: 자동 모드"); }
  else if (cmd == "CONFIG") {
    darkPct = constrain(c.dark, 0, 100);
    autoOffMs = (unsigned long)constrain(c.off, 10, 3600) * 1000;
    prefs.putInt("dark", darkPct);
    prefs.putInt("off", (int)(autoOffMs / 1000));
    Serial.printf("명령: 설정 — 어두움 기준 %d%%, 자동 꺼짐 %lu초 (저장됨)\n", darkPct, autoOffMs / 1000);
  }
  else if (cmd == "COLOR" || cmd == "BRIGHT") { Serial.println("명령: 색·밝기는 네오픽셀형(V2) 전용 — 무시"); return; }
  else { Serial.println("모르는 명령: " + cmd); return; }
  changed();
}

void handleSerial(String line) {
  line.trim();
  line.toLowerCase();
  Command c = {};
  if (line == "on")        strcpy(c.cmd, "ON");
  else if (line == "off")  strcpy(c.cmd, "OFF");
  else if (line == "auto") strcpy(c.cmd, "AUTO");
  else if (line == "s")    { printStatus(); return; }
  else if (line == "n")    { pingNow = true; return; }
  else { if (line.length()) Serial.println("명령: on / off / auto / s / n"); return; }
  applyCommand(c);
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handleSerial(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
}

// ─────────────────────────────────────────────────
// 통신 (다른 코어에서 실행)
// ─────────────────────────────────────────────────
int httpsGet(const String& url, String& body) {
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();                  // 수업용. 운영에서는 루트 인증서를 등록
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // Apps Script는 한 번 다른 주소로 넘겨줌
  if (!http.begin(client, url)) return -1000;
  int code = http.GET();
  if (code > 0) body = http.getString();
  http.end();
  return code;
}

String explainError(int code) {
  if (code == -1)  return " — 서버까지 연결 안 됨 (와이파이가 인터넷 되는지 확인)";
  if (code == -11) return " — 응답이 늦음 (잠시 후 자동 재시도)";
  if (code == 404) return " — 주소 없음 (SERVER_URL이 /exec 로 끝나는지)";
  if (code == 401 || code == 403) return " — 권한 (배포 액세스를 '모든 사용자'로)";
  return "";
}

// {"cmd":"CONFIG","value":"","dark":30,"off":300} 에서 값 하나 꺼내기
String jsonStr(const String& body, const char* key) {
  String k = String("\"") + key + "\"";
  int p = body.indexOf(k);
  if (p < 0) return "";
  int colon = body.indexOf(':', p + k.length());
  int q1 = body.indexOf('"', colon + 1);
  int comma = body.indexOf(',', colon + 1);
  if (q1 < 0 || (comma >= 0 && comma < q1)) return "";
  int q2 = body.indexOf('"', q1 + 1);
  return q2 < 0 ? "" : body.substring(q1 + 1, q2);
}

int jsonInt(const String& body, const char* key, int fallback) {
  String k = String("\"") + key + "\"";
  int p = body.indexOf(k);
  if (p < 0) return fallback;
  int colon = body.indexOf(':', p + k.length());
  return body.substring(colon + 1).toInt();
}

String reportQuery() {
  Snapshot s;
  portENTER_CRITICAL(&sharedMux);
  s = shared;
  portEXIT_CRITICAL(&sharedMux);
  return String(SERVER_URL) + "?mode=report&team=" + TEAM_ID + "&type=bulb" +
         "&power=" + (s.lampOn ? "ON" : "OFF") + "&auto=" + (s.autoMode ? 1 : 0) +
         "&light=" + s.lightPct + "&motion=" + (s.motion ? 1 : 0) +
         "&dark=" + s.darkPct + "&off=" + s.offSec;
}

// 통신 한 바퀴 — 시뮬레이터도 이 함수를 그대로 부른다
void networkStep(unsigned long now) {
  static unsigned long lastPoll = 0, lastReport = 0, lastWiFiTry = 0, failUntil = 0;
  static bool wasUp = false;

  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wasUp) {
    wasUp = up;
    Serial.println(up ? "와이파이 연결됨 — 앱 명령 대기" : "와이파이 끊김 — 자동 동작과 시리얼 명령은 계속 돼요");
    if (up) reportNow = true;
  }
  if (!up) {
    if (now - lastWiFiTry >= WIFI_RETRY_MS) { lastWiFiTry = now; WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS); }
    return;
  }
  if (now < failUntil) return;           // 실패 직후 잠깐 쉬기 (서버에 재시도 홍수 방지)

  if (pingNow) {
    pingNow = false;
    String body;
    int code = httpsGet(String(SERVER_URL) + "?mode=state&team=" + TEAM_ID, body);
    Serial.printf("서버 확인: %d%s\n", code, explainError(code).c_str());
  }

  if (reportNow || (reportSoon && now - lastReport >= SOFT_REPORT_MS) || now - lastReport >= HEARTBEAT_MS) {
    String body;
    int code = httpsGet(reportQuery(), body);
    netLastCode = code;
    if (code == 200) { reportNow = false; reportSoon = false; lastReport = now; }
    else { Serial.printf("보고 실패 %d%s\n", code, explainError(code).c_str()); failUntil = now + 5000; return; }
  }

  if (now - lastPoll >= POLL_MS && uxQueueSpacesAvailable(cmdQueue) > 0) {
    lastPoll = now;
    String body;
    int code = httpsGet(String(SERVER_URL) + "?mode=next&team=" + TEAM_ID, body);
    netLastCode = code;
    if (code != 200) { Serial.printf("명령 확인 실패 %d%s\n", code, explainError(code).c_str()); failUntil = now + 5000; return; }
    String cmd = jsonStr(body, "cmd");
    if (cmd.length()) {
      Command c = {};
      strncpy(c.cmd, cmd.c_str(), sizeof(c.cmd) - 1);
      strncpy(c.value, jsonStr(body, "value").c_str(), sizeof(c.value) - 1);
      c.dark = jsonInt(body, "dark", darkPct);
      c.off = jsonInt(body, "off", 300);
      xQueueSend(cmdQueue, &c, 0);
    }
  }
}

void networkTask(void*) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (;;) {
    networkStep(millis());
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ─────────────────────────────────────────────────
void setup() {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 깜빡임 방지: '꺼짐' 먼저
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);

  prefs.begin("moodlamp", false);
  darkPct = prefs.getInt("dark", darkPct);
  autoOffMs = (unsigned long)prefs.getInt("off", (int)(autoOffMs / 1000)) * 1000;

  lightPct = readLightPct();
  updateDarkness();
  publish();

  cmdQueue = xQueueCreate(8, sizeof(Command));
  xTaskCreatePinnedToCore(networkTask, "lamp-net", 8192, nullptr, 1, nullptr, 0);   // 통신은 코어 0
  Serial.printf("IoT 무드등 V1 [%s] 시작 — 어두움 기준 %d%%, 자동 꺼짐 %lu초\n", TEAM_ID, darkPct, autoOffMs / 1000);
}

void loop() {
  unsigned long now = millis();
  readSerial();

  Command c;
  while (xQueueReceive(cmdQueue, &c, 0) == pdTRUE) applyCommand(c);

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightPct = (lightPct * 3 + readLightPct()) / 4;     // 이동평균
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; reportSoon = true; }

  if (autoMode) {
    if (!lampOn && motion && isDark) { setRelay(true); Serial.println("자동 켜기 (어두움 + 사람)"); changed(); }
    else if (lampOn && now - lastMotionMs >= autoOffMs) { setRelay(false); Serial.println("자동 끄기 (사람 없음)"); changed(); }
  }
  publish();          // 조도처럼 자주 바뀌는 값은 보고 때 최신 것으로
}
