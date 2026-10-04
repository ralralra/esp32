/*
  2단계 — 상품 인식 + 가격 합산 (셀프 계산)
  ----------------------------------------------------
  목표: 등록된 카드를 찍으면 상품으로 인식해 가격을 더하고,
        "결제" 카드를 찍으면 합계를 보여주고 0으로 초기화한다.

  1단계에서 적어둔 UID를 아래 상품표에 넣으세요.
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

// ── 상품 표 (1단계에서 읽은 UID로 바꾸세요) ──
const int PRODUCT_COUNT = 3;
String productUID[PRODUCT_COUNT]   = { "A1B2C3D4", "11223344", "55667788" };
String productName[PRODUCT_COUNT]  = { "초코파이",   "콜라",      "사탕"     };
int    productPrice[PRODUCT_COUNT] = { 500,        1000,       300       };

// 결제(계산 완료) 카드 UID
String CHECKOUT_UID = "99AABBCC";

int total = 0;   // 현재까지 합계

String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("\n=== 매점 셀프 계산 ===");
  Serial.println("상품 카드를 찍고, 끝나면 [결제] 카드를 찍으세요.");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readUID();

  // 결제 카드인가?
  if (uid == CHECKOUT_UID) {
    Serial.printf("===== 합계: %d원 =====\n\n", total);
    total = 0;   // 다음 손님을 위해 초기화
  } else {
    // 상품표에서 찾기
    bool found = false;
    for (int i = 0; i < PRODUCT_COUNT; i++) {
      if (uid == productUID[i]) {
        total += productPrice[i];
        Serial.printf("%s  +%d원   (합계 %d원)\n",
                      productName[i].c_str(), productPrice[i], total);
        found = true;
        break;
      }
    }
    if (!found) Serial.println("등록되지 않은 카드: " + uid);
  }

  rfid.PICC_HaltA();
  delay(800);
}
