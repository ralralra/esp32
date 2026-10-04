# V2 · 3단계 — 앱·음성 연동 완성 펌웨어

> ✅ 코드: [`step3_app_mood_lamp.ino`](step3_app_mood_lamp.ino) — PC 시뮬레이터(가짜 서버 포함)로 검증 · ESP32 컴파일 확인(esp32 코어 3.3.12, 경고 없음)

## 업로드 전에 바꿀 4줄

```cpp
const char* WIFI_SSID  = "와이파이이름";       // 2.4GHz만
const char* WIFI_PASS  = "와이파이비밀번호";
const char* SERVER_URL = "https://script.google.com/macros/s/배포ID/exec";   // 선생님이 준 주소
const char* TEAM_ID    = "TEAM01";             // 내 팀 번호 — 12팀이 모두 달라야 해요
```

라이브러리: **Adafruit NeoPixel** (라이브러리 관리에서 설치)
⚠ Wemos D1 R32를 쉴드 없이 직결했다면 `MAX_BRIGHTNESS`를 128 → **64**로 → [`../../../docs/board_wemos.md`](../../../docs/board_wemos.md)

## 동작

- V1 4단계와 같은 통신 구조 + `COLOR` · `BRIGHT`
- 색: `warm` `white` `red` `orange` `yellow` `green` `blue` `purple` `pink` `rainbow`, 또는 6자리 색 코드(`FF8800`)
- `rainbow`: 링 둘레마다 다른 색이 천천히 돌아감 (약 10초에 한 바퀴)
- 켜고 끌 때 1초 동안 서서히 · 다시 켜면 마지막 색·밝기 그대로
- 밝기 명령이 연달아 와도 서버가 마지막 하나만 넘겨줌

## 시리얼 모니터 (115200)

`on` · `off` · `auto` · `s` · `n` · `c red` / `c FF8800` / `c rainbow` · `b 50`

PC 시뮬레이션: `cd ../../../sim && make run` (`sim_v2_app` — 15가지 확인)
