/*
  IoT 무드등 V2 · 2단계 — 출력 테스트 (릴레이 → 220V 전구)

  보드: Wemos D1 R32 + 센서쉴드 V5  (배선: ../../wiring.md)
    릴레이 모듈(KY-019형, 5V · HIGH 트리거) → 실드 2번 (S→S, +→V, −→G) = GPIO26
    SEL 점퍼 꽂음 (보드 5V 사용), 보드는 micro USB 5V 1A 충전기

  하는 일
    시리얼로 릴레이를 붙였다 떼며 '딸깍' 소리와 모듈 LED를 확인한다.
    ⚠ 처음에는 220V 없이 릴레이만 연결. 전구 쪽 220V 배선은 교사가 한 뒤 다시 확인.

  시리얼(115200) 명령
    1 / on     릴레이 켜기(붙음)      0 / off   끄기(떨어짐)
    t          토글                  blink     3번 깜빡
*/

const int  RELAY_PIN        = 26;     // 실드 2번
const bool RELAY_ACTIVE_LOW = false;  // KY-019형(HIGH에서 켜짐) = false · LOW에서 켜지는 모듈이면 true

bool relayOn = false;
String inputLine;

void setRelay(bool on) {
  relayOn = on;
  digitalWrite(RELAY_PIN, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
  Serial.println(on ? "릴레이 ON (딸깍 · 모듈 LED 켜짐)" : "릴레이 OFF");
}

void handle(String line) {
  line.trim();
  line.toLowerCase();
  if (line == "1" || line == "on")       setRelay(true);
  else if (line == "0" || line == "off") setRelay(false);
  else if (line == "t")                  setRelay(!relayOn);
  else if (line == "blink") { for (int i = 0; i < 3; i++) { setRelay(true); delay(500); setRelay(false); delay(500); } }
  else if (line.length())                Serial.println("명령: 1 / 0 / t / blink");
}

void setup() {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);   // 부팅 순간 깜빡임 방지: '꺼짐' 값을 먼저
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("릴레이 테스트 — 1(켜기) / 0(끄기) / t(토글) / blink");
  Serial.println("보드 리셋 순간에 릴레이가 붙지 않아야 정상 · HIGH에서 켜지지 않으면 RELAY_ACTIVE_LOW = true");
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handle(inputLine); inputLine = ""; }
    else inputLine += ch;
  }
}
