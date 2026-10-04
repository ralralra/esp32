/*
  1단계 — RFID 카드 고유번호(UID) 읽기
  ----------------------------------------------------
  목표: 카드를 리더에 대면 그 카드의 고유번호(UID)가 시리얼에 뜬다.
        각 상품 카드의 UID를 적어두면, 2단계에서 상품으로 등록한다.

  라이브러리: MFRC522 (Library Manager에서 설치)
  배선: SDA→5, SCK→18, MOSI→23, MISO→19, RST→27, 3.3V→3.3V, GND→GND
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 27

MFRC522 rfid(SS_PIN, RST_PIN);

// 카드 UID를 읽어서 "A1B2C3D4" 같은 문자열로 만들어 반환
String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";   // 한 자리면 0 채우기
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();          // SPI 통신 시작
  rfid.PCD_Init();      // RFID 리더 켜기
  Serial.println("\n=== RFID UID 읽기 ===");
  Serial.println("카드를 리더에 대보세요. 각 카드의 UID를 적어두세요.");
}

void loop() {
  // 새 카드가 있는지 확인
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readUID();
  Serial.println("카드 UID: " + uid);

  rfid.PICC_HaltA();    // 카드 통신 종료 (다음 카드 준비)
  delay(800);           // 같은 카드 연속 인식 방지
}

/*
  각 카드 UID를 적어두자 (2단계에서 상품 표에 입력):
    초코파이  UID = ____________
    콜라      UID = ____________
    사탕      UID = ____________
    [결제]    UID = ____________   ← 계산 완료용 카드 1장
*/
