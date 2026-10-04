/*
  0단계 — RC522 배선 확인 + 학생증 UID 읽기 테스트
  ----------------------------------------------------
  목표: 학생증을 리더기에 대면 시리얼 모니터(115200)에 UID가 찍힌다.

  이걸로 확인하는 것:
    1) 배선이 맞는지 (docs/wiring.md 참고)
    2) 우리 학교 학생증이 RC522로 읽히는 카드인지 (ISO 14443 Type A)
       → 안 찍히면 Type B/FeliCa 카드일 수 있음. 다른 카드로 테스트해 볼 것.

  준비: Arduino IDE → 라이브러리 매니저 → "MFRC522" (by GithubCommunity) 설치
  보드: ESP32 Dev Module (WROOM-32)
*/

#include <SPI.h>
#include <MFRC522.h>

#define PIN_SS   5    // RC522 SDA(SS)
#define PIN_RST  27   // RC522 RST

// ── 송신 출력 부스트 실험 ──
// 일부 카드(안테나가 작은 학생증·유스카드 등)는 RC522 기본 출력으로는 깨어나지 않는다.
// 1 = 안테나 구동 출력을 최대로 올림 (안 되던 카드가 될 수 있음)
// 0 = 기본 출력 (부스트 후 잘 되던 카드까지 안 되면 0으로 되돌릴 것)
#define BOOST_FIELD 1

MFRC522 rfid(PIN_SS, PIN_RST);

void setup() {
  Serial.begin(115200);
  SPI.begin();          // SCK=18, MISO=19, MOSI=23 (ESP32 기본 SPI 핀)
  rfid.PCD_Init();
  delay(100);
  // 안테나 감도를 최대로 (기본값은 중간 — 클론 모듈은 이걸 안 올리면 인식 거리가 매우 짧음)
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);

#if BOOST_FIELD
  // 안테나 구동 전도도(=송신 세기)를 끌어올린다.
  // GsNReg 상위 4비트 = 반송파 세기(최대 F), 하위 4비트 = 변조 세기(기본 8 유지)
  rfid.PCD_WriteRegister(MFRC522::GsNReg, 0xF8);    // 기본 0x88 → 반송파 최대
  rfid.PCD_WriteRegister(MFRC522::CWGsPReg, 0x3F);  // 기본 0x20 → p드라이버 반송파 최대
  Serial.println("(송신 출력 부스트 ON)");
#endif

  // 리더기가 응답하는지 자가진단 (배선이 틀리면 0x00 또는 0xFF가 나옴)
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.println();
  Serial.print("RC522 버전 레지스터: 0x");
  Serial.println(version, HEX);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("⚠️ 리더기 응답 없음! 배선을 다시 확인하세요 (특히 3.3V, SDA=5, RST=27)");
  } else {
    Serial.println("✅ 리더기 연결 OK. 학생증을 대보세요.");
  }
}

void loop() {
  // REQA 대신 WUPA(깨우기)로 카드를 찾는다.
  // 학생증 같은 보안 스마트카드(ISO 14443-4)는 일반 감지(REQA)에 응답하지 않거나
  // HALT 상태에 머무는 경우가 있어서, WUPA 방식이 훨씬 안정적으로 잡는다.
  byte atqa[2];
  byte atqaSize = sizeof(atqa);
  MFRC522::StatusCode st = rfid.PICC_WakeupA(atqa, &atqaSize);
  if (st != MFRC522::STATUS_OK && st != MFRC522::STATUS_COLLISION) {
    delay(50);
    return;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // UID를 "A1B2C3D4" 처럼 대문자 16진수 문자열로 만든다
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("카드 인식! UID = ");
  Serial.print(uid);
  Serial.print("  (");
  Serial.print(rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));
  Serial.println(")");

  rfid.PICC_HaltA();        // 카드와의 통신 종료
  rfid.PCD_StopCrypto1();
  delay(1000);              // 같은 카드 연속 인식 방지
}
