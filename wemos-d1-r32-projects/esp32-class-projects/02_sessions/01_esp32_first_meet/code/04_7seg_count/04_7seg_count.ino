/*
  7세그 카운트 — '갱신'의 기본기
  변수에 담긴 숫자를 1초마다 새로 표시해요.
  이 코드가 잠시 뒤 '카운트다운 타이머'의 심장이 됩니다!
  연결: images/wiring_7segment.png (G→GND, V→3V3, DIO→D6 핀, CLK→D7 핀)
*/

#include <TM1637Display.h>

#define DIO 27  // D6 핀
#define CLK 14  // D7 핀

TM1637Display display(CLK, DIO);

int count = 0;   // 변수: 숫자를 담는 상자

void setup() {
  display.setBrightness(7);
}

void loop() {
  display.showNumberDec(count);  // 상자 속 숫자를 표시
  count = count + 1;             // 상자 속 숫자를 1 키우고
  delay(1000);                   // 1초 쉬고 → 다시 loop 처음으로!
}
