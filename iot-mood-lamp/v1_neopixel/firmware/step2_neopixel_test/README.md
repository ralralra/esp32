# V1 · 2단계 — 출력 테스트 (네오픽셀 스트립 16구)

> ✅ 코드: [`step2_neopixel_test.ino`](step2_neopixel_test.ino) — ESP32 컴파일 확인 완료
> 라이브러리: **Adafruit NeoPixel** (라이브러리 관리에서 설치)

## 목표
스트립만 연결해 **색 순서 · 밝기 · 웜화이트** 가 제대로 나오는지 확인한다.

## 진행
1. 배선 → [`../../wiring.md`](../../wiring.md). **SEL 점퍼를 빼고 EXT PWR에 5V** (PC USB만 꽂으면 네오픽셀에 전원이 안 감)
2. 업로드하면 동작 확인된 샘플(strandtest)과 똑같은 **무지개가 스트립을 따라 흐른다.** 설정도 샘플 그대로이다.
   ```cpp
   Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);   // 16, 27
   strip.setBrightness(50);
   ```
   - **초록·빨강·파랑·꺼짐이 한 칸씩 번갈아** 멈춰 있으면 RGBW 설정(`NEO_GRBW`)으로 보낸 것 → `NEO_GRB`로
   - 끝쪽 LED가 안 켜지거나 앞쪽만 켜지면 `LED_COUNT`를 자른 개수에 맞춘다
3. 시리얼(115200)로 바꿔 본다: `c red` · `c FF8800` · `rainbow` · `chase`(샘플의 theaterChaseRainbow) · `b 50`(0~255) · `test` · `off` · `on`
   - `test`는 빨강 → 초록 → 파랑을 차례로 켠다. 이름과 색이 다르면 `NEO_GRB`를 `NEO_RGB`로
4. 밤에 램프를 켜 두고 **조명으로 충분히 밝은지** 본다. `b 128` · `b 255`로 올려 보고, 3·4단계의 `MAX_BRIGHTNESS`(기본 128)를 정한다.

## 확인
- [ ] 무지개가 16개 LED 전체에 걸쳐 흐름 (`LED_COUNT 16`, `NEO_GRB`)
- [ ] `test`에서 빨강이 빨강으로 나옴 (색 순서)
- [ ] `c warm`(주황빛 흰색)이 `c white`보다 따뜻하게 보임
- [ ] 흰색 최대 밝기(`c white` + `b 255`)에서 보드가 리셋되지 않음
