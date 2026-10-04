/*
  (옵션) LCD 카운트다운 타이머 — 본 과정 미사용! 수업용은 06_countdown_timer(7세그)·09_7segment_timer를 쓰세요.

  LCD: 일반 I2C LCD 16×2 (PCF8574 백팩) — 주소는 보통 0x27 (또는 0x3F), 00_i2c_scanner로 확인!

  연결: LCD → 점퍼선 4가닥 — GND→GND, VCC→5V(백팩 기준), SDA→보드 SDA 핀(GPIO21), SCL→보드 SCL 핀(GPIO22)
        빛 센서 → G→GND, V→3V3, S→A2~A5 핀
  라이브러리: LiquidCrystal I2C
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED   2    // 내장 LED
#define LIGHT 34   // 빛센서 (A3 핀 예시 — 심화용, 없으면 관련 줄 삭제)
#define DARK  1000 // '가려짐' 기준값 — 시리얼로 관찰해서 교실에 맞게 조절

LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C LCD 주소 = 0x27 (안 되면 0x3F — 00_i2c_scanner로 확인)

void setup() {
  pinMode(LED, OUTPUT);
  Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22 (보드의 SDA·SCL 핀)
  lcd.init();
  lcd.backlight();

  for (int i = 10; i >= 0; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timer:");
    lcd.setCursor(0, 1);
    lcd.print(i);
    delay(1000);

    // 심화: 가려져 있는 동안은 기다리기(일시정지)
    while (analogRead(LIGHT) < DARK) {
      lcd.setCursor(8, 0);
      lcd.print("PAUSE");
      delay(200);
    }
    lcd.setCursor(8, 0);
    lcd.print("     ");  // PAUSE 글자 지우기
  }

  // 0이 된 순간!
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TIME OVER!");
  for (int i = 0; i < 5; i++) {   // LED 빠르게 5번 점멸
    digitalWrite(LED, HIGH);
    delay(150);
    digitalWrite(LED, LOW);
    delay(150);
  }
}

void loop() { }
