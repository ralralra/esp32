/*
  IoT 무드등 · 1단계 — 센서 테스트 (조도 + 인체감지)   ※ V1 네오픽셀 · V2 릴레이 공통

  보드: Wemos D1 R32 + 아두이노 센서쉴드 V5  (배선: ../../wiring.md)
    인체감지 PIR(HC-SR501) → 실드 3번 (OUT→S, VCC→V, GND→G)          = GPIO25
    조도센서(LDR 모듈)     → 실드 A3 (AO→S, GND→G), VCC는 Bluetooth 헤더 3V3 = GPIO34

  하는 일
    1초마다 조도 원시값(0~4095)·밝기 %·'어두움/밝음' 판정과 PIR 값을 시리얼에 찍는다.
    PIR이 바뀌는 순간은 바로 알린다.

  이 단계에서 정할 것 → 아래 숫자 3개 (3·4단계에 똑같이 적는다)
    DARK_LEVEL   : 밝기 %가 이 숫자보다 낮으면 '어두움' → 램프 켜짐 / 이보다 밝아지면 꺼짐
    DARK_IS_HIGH : 이번 조도 모듈은 어두울수록 값이 커서 true. 가렸는데 '밝기 %'가 올라가면 false (모듈마다 방향이 다름)
    (PIR)        : 모듈의 감도·유지시간 다이얼은 이 화면을 보며 맞춘다

  시리얼 모니터: 115200
*/

// ── 핀 ───────────────────────────────────────────
const int PIR_PIN   = 25;   // 실드 3번
const int LIGHT_PIN = 34;   // 실드 A3 (ADC1 — 와이파이를 켜도 읽힘)

// ── 조도 기준 — 여기만 정하면 됨 ────────────────────
const int  DARK_LEVEL   = 30;     // 주변 밝기(%)가 이 숫자보다 낮으면 '어두움'
const int  DARK_MARGIN  = 5;      // 다시 '밝음'이 되려면 기준 + 5% 이상 (경계에서 깜빡임 방지 — 보통 그대로)
const bool DARK_IS_HIGH = true;   // 이 조도 모듈은 어두울수록 값이 커짐 — 가렸는데 %가 내려가면 false

const unsigned long PRINT_MS = 1000;

bool isDark = false;
bool lastMotion = false;
int  lightAvg = 0;
unsigned long lastPrintMs = 0;
unsigned long lastMotionMs = 0;

int lightPercent(int raw) {
  int pct = (long)raw * 100 / 4095;         // 0~4095 → 0~100
  return DARK_IS_HIGH ? 100 - pct : pct;    // 100 = 밝음, 0 = 깜깜
}

void updateDarkness(int pct) {
  if (!isDark && pct < DARK_LEVEL)               isDark = true;    // 기준보다 어두워짐
  if (isDark  && pct >= DARK_LEVEL + DARK_MARGIN) isDark = false;  // 기준(+여유)보다 밝아짐
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  lightAvg = analogRead(LIGHT_PIN);
  Serial.println("센서 테스트 시작 — PIR은 전원 후 30~60초 동안 값이 불안정해요");
  Serial.printf("어두움 기준 %d%% (밝음으로 복귀 %d%%)\n", DARK_LEVEL, DARK_LEVEL + DARK_MARGIN);
}

void loop() {
  unsigned long now = millis();

  lightAvg = (lightAvg * 3 + analogRead(LIGHT_PIN)) / 4;   // 이동평균 — 값이 덜 튄다
  int pct = lightPercent(lightAvg);
  updateDarkness(pct);

  bool motion = digitalRead(PIR_PIN) == HIGH;
  if (motion) lastMotionMs = now;
  if (motion != lastMotion) {
    lastMotion = motion;
    Serial.println(motion ? "  PIR ▲ 감지됨" : "  PIR ▽ 해제");
  }

  if (now - lastPrintMs >= PRINT_MS) {
    lastPrintMs = now;
    Serial.printf("조도 원시값=%4d  밝기=%3d%%  → %s (기준 %d%%)  |  PIR=%d  마지막 감지 %lu초 전\n",
                  lightAvg, pct, isDark ? "어두움 → 켜짐" : "밝음   → 꺼짐", DARK_LEVEL,
                  motion, (now - lastMotionMs) / 1000);
  }
  delay(50);
}
