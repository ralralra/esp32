/*
  I2C 스캐너 — I2C 장치가 안 잡힐 때 '진짜 주소'를 찾아주는 탐정 코드
  I2C 모듈을 점퍼선 4가닥으로 연결한 채 업로드 → 시리얼 모니터(115200) 확인!
    GND → GND, VCC → 3V3(5V 전용 LCD 백팩이면 5V), SDA → SDA 핀(GPIO21), SCL → SCL 핀(GPIO22)

  결과 읽는 법:
    "발견! 주소 = 0x27"  → 코드의 lcd(0x27, 16, 2) 그대로 OK
    "발견! 주소 = 0x3F"  → lcd(0x3F, 16, 2) 로 바꾸기
    "아무것도 못 찾음"    → 주소 문제가 아니라 배선·전원 문제!
                           (점퍼선 빠짐, SDA·SCL 뒤바뀜, GND·VCC 연결 확인)

  ⚠ 우노의 A4·A5 자리는 D1 R32에선 입력 전용 핀(GPIO 36·39)이라 I2C로 못 써요.
    반드시 보드의 SDA(21)·SCL(22) 핀에 연결하세요.
    다른 핀(예: D6·D7)에 연결했다면 아래를 Wire.begin(27, 14); 처럼 바꿔 다시 스캔!
*/

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();          // 기본: SDA=21, SCL=22 / D6·D7 핀에 연결했다면 Wire.begin(27, 14);
  delay(1000);
  Serial.println("\nI2C 스캔 시작...");
}

void loop() {
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {   // 응답이 오면 장치 존재!
      Serial.printf("발견! 주소 = 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0)
    Serial.println("아무것도 못 찾음 — 배선·전원을 확인하세요");
  Serial.println("--- 3초 뒤 다시 스캔 ---");
  delay(3000);
}
