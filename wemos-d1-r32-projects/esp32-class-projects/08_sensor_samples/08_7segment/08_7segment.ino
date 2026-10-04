/*
  7세그먼트 숫자표시기(TM1637) — 기본 사용법
  연결: 점퍼선 4가닥 (그림: images/wiring_7segment.png)
       GND→GND, VCC→3V3, DIO→D6 핀(GPIO27), CLK→D7 핀(GPIO14)
  라이브러리: 라이브러리 관리에서 'TM1637' 검색 → TM1637 by Avishay Orpaz
  ⚠ 숫자가 안 나오면 DIO·CLK를 서로 바꿔 시도!
*/

#include <TM1637Display.h>

#define DIO 27  // D6 핀
#define CLK 14  // D7 핀

TM1637Display display(CLK, DIO);

void setup() {
  display.setBrightness(7);  // 밝기 0(어둡게)~7(밝게)
}

void loop() {
  display.showNumberDec(1234);              // 그냥 숫자
  delay(2000);

  display.showNumberDecEx(1234, 0b01000000); // 가운데 콜론(:) 켜기 → 12:34
  delay(2000);

  for (int i = 0; i <= 9; i++) {             // 0~9 세기
    display.showNumberDec(i);
    delay(300);
  }
  display.clear();
  delay(500);
}
