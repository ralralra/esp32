# V1 · 4단계 — 앱·음성 연동 완성 펌웨어 (네오픽셀)

> ✅ 코드: [`step4_app_mood_lamp.ino`](step4_app_mood_lamp.ino) — 동작 검증·ESP32 컴파일 확인 완료
> 라이브러리: **Adafruit NeoPixel**

## 업로드 전에 바꿀 것

```cpp
const char* WIFI_SSID  = "와이파이이름";       // 2.4GHz만
const char* WIFI_PASS  = "와이파이비밀번호";
const char* SERVER_URL = "https://script.google.com/macros/s/배포ID/exec";   // 선생님이 준 주소
const char* TEAM_ID    = "TEAM01";             // 내 팀 번호 — 12팀이 모두 달라야 해요

const int  DARK_LEVEL   = 30;      // 1단계에서 정한 숫자
const bool DARK_IS_HIGH = false;   // 1단계에서 확인한 방향
```

## 동작
- 자동 모드는 3단계와 같은 규칙. 앱 자동화 탭의 **어두움 기준 %·꺼지는 시간**은 보드에 저장돼 껐다 켜도 유지
- 2초마다 서버에서 **내 팀 명령**을 하나씩 가져와 실행 — `ON` · `OFF` · `AUTO` · `COLOR` · `BRIGHT` · `CONFIG`
- 색: `warm` `white` `red` `orange` `yellow` `green` `blue` `purple` `pink` `rainbow`, 또는 6자리 색 코드(`FF8800`)
- 켜짐·모드·색이 바뀌면 바로, 움직임은 5초에 한 번까지, 그 밖에는 30초마다 상태 보고 (`type=neopixel`)
- 통신은 **다른 코어**에서 돌아서, 와이파이가 느리거나 끊겨도 자동 동작·페이드는 계속

## 시리얼 모니터 (115200)
`on` · `off` · `auto` · `s`(상태) · `n`(서버 연결 확인 — 응답 코드와 원인) · `c red` / `c FF8800` · `b 50`

## 확인
- [ ] 시리얼에 "와이파이 연결됨" → 시트 `state` 탭 내 팀 행에 `neopixel`이 채워짐
- [ ] 앱 '켜기' → 2~3초 안에 켜짐 · 프리셋 색 → 색 바뀜 · 슬라이더 → 밝기 바뀜
- [ ] 앱 자동화 탭에서 설정 저장 → 시리얼에 "설정 — 어두움 기준 …" 출력
- [ ] 공유기 전원을 내려도 자동 모드 유지
