/*
  IoT 무드등 V2 · 5단계 — 앱·음성 연동 '빠른 응답' 펌웨어 (릴레이 전구 · MQTT)

  4단계(구글시트 폴링, 1~3초 지연) 대신 MQTT 브로커로 명령을 '밀어받아' 0.1~0.5초 안에 반응합니다.
  센서·자동 동작 코드는 3·4단계와 같고, 통신 부분만 다릅니다.

  ① 아래 "여기만 바꾸기"를 고친 뒤 업로드하세요.
     WIFI_SSID / WIFI_PASS    : 2.4GHz 와이파이
     MQTT_HOST / PORT / USER / PASS : 선생님이 만든 브로커 (docs/mqtt.md) — 공개 브로커면 USER·PASS 비움
     TOPIC_PREFIX             : 우리 반 고유 문자열 (예: moodlamp/3반-a7x9) — 반마다 다르게, 공개 브로커면 꼭 임의 문자열
     TEAM_ID                  : 내 팀 번호 TEAM01 ~ TEAM12
  ② 1단계에서 정한 DARK_LEVEL · DARK_IS_HIGH 를 3단계와 똑같이 적으세요.

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    릴레이(KY-019형, HIGH 트리거) → 실드 2번 (GPIO26)   PIR → 실드 3번 (GPIO25)   조도 → A3 (GPIO34, VCC는 3V3)
  라이브러리: PubSubClient (라이브러리 관리에서 설치)

  토픽 (TOPIC_PREFIX/TEAM_ID/… )
    …/cmd     ← 앱이 보냄: ON · OFF · AUTO · CONFIG 30 300   (COLOR·BRIGHT는 무시)
    …/state   → 보드가 보냄 (retained): {"type":"bulb","power":"ON","auto":1,"light":32,"motion":1,"dark":30,"off":300}
    …/status  → online / offline (retained, 보드가 끊기면 브로커가 offline으로 바꿈)

  시리얼(115200) 명령: on / off / auto / s(상태)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>

// ── ① 여기만 바꾸기 ─────────────────────────────────
const char* WIFI_SSID    = "와이파이이름";
const char* WIFI_PASS    = "와이파이비밀번호";
const char* MQTT_HOST    = "xxxxxxxx.ala.asia-southeast1.emqxsl.com";   // 브로커 주소 (docs/mqtt.md)
const int   MQTT_PORT    = 8883;                 // TLS 8883 · 공개 브로커를 TLS 없이 쓰면 1883 + MQTT_TLS false
const bool  MQTT_TLS     = true;
const char* MQTT_USER    = "moodlamp";           // 브로커에서 만든 계정 (공개 브로커면 "")
const char* MQTT_PASS    = "비밀번호";
const char* TOPIC_PREFIX = "moodlamp/3반-a7x9";  // 우리 반 고유 문자열
const char* TEAM_ID      = "TEAM01";

// ── ② 조도 기준 — 1단계에서 정한 숫자 ───────────────────
const int  DARK_LEVEL   = 30;
const int  DARK_MARGIN  = 5;
const bool DARK_IS_HIGH = true;
const bool USE_PIR      = true;

// ── 핀 ───────────────────────────────────────────
const int RELAY_PIN = 26;   // 실드 2번
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3
const bool RELAY_ACTIVE_LOW = false;  // KY-019형(HIGH에서 켜짐) = false

// ── 통신 간격 ────────────────────────────────────────
const unsigned long HEARTBEAT_MS   = 30000;
const unsigned long SOFT_REPORT_MS = 5000;
const unsigned long RECONNECT_MS   = 5000;
const unsigned long WIFI_RETRY_MS  = 10000;
const unsigned long LIGHT_READ_MS  = 200;

// ── 램프 상태 (loop 전용) ─────────────────────────────
bool lampOn = false, autoMode = true, isDark = false, motion = false;
int  lightPct = 0;
int  darkPct = DARK_LEVEL;
unsigned long autoOffMs = 300000;
unsigned long lastMotionMs = 0, lastLightMs = 0;
String inputLine;
Preferences prefs;

struct Command { char cmd[10]; char value[20]; int dark; int off; };
struct Snapshot { bool lampOn, autoMode, motion; int lightPct, darkPct, offSec; };
QueueHandle_t cmdQueue;
Snapshot shared;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool reportNow = true;
volatile bool reportSoon = false;
volatile int  netState = 0;        // 0 와이파이 없음 · 1 브로커 연결 중 · 2 연결됨

String topicCmd, topicState, topicStatus;

// ─────────────────────────────────────────────────
void setRelay(bool on) {
  digitalWrite(RELAY_PIN, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
  lampOn = on;
}

int readLightPct() {
  int pct = (long)analogRead(LIGHT_PIN) * 100 / 4095;
  return DARK_IS_HIGH ? 100 - pct : pct;
}

void updateDarkness() {
  if (!isDark && lightPct < darkPct)                isDark = true;
  if (isDark  && lightPct >= darkPct + DARK_MARGIN) isDark = false;
}

void publish() {
  Snapshot s = { lampOn, autoMode, motion, lightPct, darkPct, (int)(autoOffMs / 1000) };
  portENTER_CRITICAL(&sharedMux);
  shared = s;
  portEXIT_CRITICAL(&sharedMux);
}

void changed() { publish(); reportNow = true; }

void printStatus() {
  Serial.printf("[%s] 전구=%s 모드=%s | 주변=%d%% %s(기준 %d%%) PIR=%d 자동꺼짐=%lus 브로커=%s\n",
                TEAM_ID, lampOn ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL", lightPct,
                isDark ? "어두움" : "밝음", darkPct, motion, autoOffMs / 1000,
                netState == 2 ? "연결됨" : (netState == 1 ? "연결 중" : "와이파이 없음"));
}

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
  else if (cmd == "COLOR" || cmd == "BRIGHT") { Serial.println("명령: 색·밝기는 네오픽셀형(V1) 전용 — 무시"); return; }
  else { Serial.println("모르는 명령: " + cmd); return; }
  changed();
}

// "CONFIG 30 300" · "ON" 같은 한 줄을 Command로
bool parseCommand(const String& line, Command& c) {
  c = Command{};
  String s = line; s.trim();
  int sp = s.indexOf(' ');
  String head = sp < 0 ? s : s.substring(0, sp);
  String rest = sp < 0 ? "" : s.substring(sp + 1);
  head.toUpperCase(); rest.trim();
  if (!head.length() || head.length() >= sizeof(c.cmd)) return false;
  strncpy(c.cmd, head.c_str(), sizeof(c.cmd) - 1);
  if (head == "CONFIG") {
    int sp2 = rest.indexOf(' ');
    c.dark = rest.substring(0, sp2 < 0 ? rest.length() : sp2).toInt();
    c.off  = sp2 < 0 ? 300 : rest.substring(sp2 + 1).toInt();
  } else strncpy(c.value, rest.c_str(), sizeof(c.value) - 1);
  return true;
}

void handleSerial(String line) {
  line.trim();
  line.toLowerCase();
  Command c;
  if (line == "s") { printStatus(); return; }
  if (line == "on" || line == "off" || line == "auto") { if (parseCommand(line, c)) applyCommand(c); return; }
  if (line.length()) Serial.println("명령: on / off / auto / s");
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handleSerial(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
}

// ─────────────────────────────────────────────────
// 통신 (다른 코어) — MQTT
// ─────────────────────────────────────────────────
WiFiClientSecure tlsClient;
WiFiClient       plainClient;
PubSubClient     mqtt;

void onMessage(char* topic, byte* payload, unsigned int len) {
  String line;
  for (unsigned i = 0; i < len && i < 60; i++) line += (char)payload[i];
  Command c;
  if (parseCommand(line, c)) xQueueSend(cmdQueue, &c, 0);
}

String stateJson() {
  Snapshot s;
  portENTER_CRITICAL(&sharedMux);
  s = shared;
  portEXIT_CRITICAL(&sharedMux);
  char buf[160];
  snprintf(buf, sizeof(buf),
           "{\"type\":\"bulb\",\"power\":\"%s\",\"auto\":%d,\"light\":%d,\"motion\":%d,\"dark\":%d,\"off\":%d}",
           s.lampOn ? "ON" : "OFF", s.autoMode ? 1 : 0, s.lightPct, s.motion ? 1 : 0, s.darkPct, s.offSec);
  return String(buf);
}

bool connectBroker() {
  String clientId = String(TEAM_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  bool ok = mqtt.connect(clientId.c_str(), MQTT_USER[0] ? MQTT_USER : nullptr, MQTT_PASS[0] ? MQTT_PASS : nullptr,
                         topicStatus.c_str(), 0, true, "offline");
  if (!ok) return false;
  mqtt.subscribe(topicCmd.c_str());
  mqtt.publish(topicStatus.c_str(), "online", true);
  return true;
}

String explainMqtt(int st) {
  if (st == -2) return " — 브로커까지 연결 안 됨 (주소·포트·TLS, 와이파이 인터넷 확인)";
  if (st == 5)  return " — 인증 실패 (MQTT_USER·MQTT_PASS, 공개 브로커면 비움)";
  if (st == 4)  return " — 계정 정보가 틀림";
  if (st == -4) return " — 응답 없음 (잠시 후 재시도)";
  return "";
}

void networkStep(unsigned long now) {
  static unsigned long lastReport = 0, lastWiFiTry = 0, lastConnectTry = 0;
  static bool wasUp = false, wasConnected = false, triedOnce = false;

  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wasUp) {
    wasUp = up;
    Serial.println(up ? "와이파이 연결됨 — 브로커 연결 중" : "와이파이 끊김 — 자동 동작과 시리얼 명령은 계속 돼요");
  }
  if (!up) {
    netState = 0;
    if (now - lastWiFiTry >= WIFI_RETRY_MS) { lastWiFiTry = now; WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS); }
    return;
  }

  if (!mqtt.connected()) {
    if (wasConnected) { wasConnected = false; Serial.println("브로커 끊김 — 다시 연결합니다"); }
    netState = 1;
    if (triedOnce && now - lastConnectTry < RECONNECT_MS) return;   // 첫 시도는 바로, 그다음은 5초마다
    triedOnce = true;
    lastConnectTry = now;
    if (connectBroker()) { wasConnected = true; netState = 2; reportNow = true; Serial.println("브로커 연결됨 — 앱 명령 대기 (" + topicCmd + ")"); }
    else Serial.printf("브로커 연결 실패 %d%s\n", mqtt.state(), explainMqtt(mqtt.state()).c_str());
    return;
  }

  mqtt.loop();
  if (reportNow || (reportSoon && now - lastReport >= SOFT_REPORT_MS) || now - lastReport >= HEARTBEAT_MS) {
    if (mqtt.publish(topicState.c_str(), stateJson().c_str(), true)) { reportNow = false; reportSoon = false; lastReport = now; }
  }
}

void networkTask(void*) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (;;) {
    networkStep(millis());
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ─────────────────────────────────────────────────
void setup() {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 깜빡임 방지
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);

  prefs.begin("moodlamp", false);
  darkPct = prefs.getInt("dark", darkPct);
  autoOffMs = (unsigned long)prefs.getInt("off", (int)(autoOffMs / 1000)) * 1000;

  lightPct = readLightPct();
  updateDarkness();
  publish();

  String base = String(TOPIC_PREFIX) + "/" + TEAM_ID + "/";
  topicCmd = base + "cmd"; topicState = base + "state"; topicStatus = base + "status";
  if (MQTT_TLS) { tlsClient.setInsecure(); mqtt.setClient(tlsClient); }
  else mqtt.setClient(plainClient);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  cmdQueue = xQueueCreate(8, sizeof(Command));
  xTaskCreatePinnedToCore(networkTask, "lamp-mqtt", 8192, nullptr, 1, nullptr, 0);
  Serial.printf("IoT 무드등 V2 릴레이 MQTT [%s] 시작 — 어두움 기준 %d%%, 자동 꺼짐 %lu초, PIR %s\n",
                TEAM_ID, darkPct, autoOffMs / 1000, USE_PIR ? "사용" : "사용 안 함");
}

void loop() {
  unsigned long now = millis();
  readSerial();

  Command c;
  while (xQueueReceive(cmdQueue, &c, 0) == pdTRUE) applyCommand(c);

  if (now - lastLightMs >= LIGHT_READ_MS) {
    lastLightMs = now;
    lightPct = (lightPct * 3 + readLightPct()) / 4;
    updateDarkness();
  }

  bool m = digitalRead(PIR_PIN) == HIGH;
  if (m) lastMotionMs = now;
  if (m != motion) { motion = m; reportSoon = true; }

  if (autoMode) {
    bool present = !USE_PIR || motion;
    if (!lampOn && isDark && present)                                { setRelay(true);  Serial.println("자동 켜기 (어두움)"); changed(); }
    else if (lampOn && !isDark)                                      { setRelay(false); Serial.println("자동 끄기 (밝아짐)"); changed(); }
    else if (lampOn && USE_PIR && now - lastMotionMs >= autoOffMs)   { setRelay(false); Serial.println("자동 끄기 (사람 없음)"); changed(); }
  }
  publish();
}
