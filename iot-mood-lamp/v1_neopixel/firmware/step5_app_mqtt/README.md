# V1 · 5단계 — 앱·음성 연동 '빠른 응답' 펌웨어 (네오픽셀 · MQTT)

> ✅ 코드: [`step5_app_mqtt.ino`](step5_app_mqtt.ino) — 동작 검증·ESP32 컴파일 확인 완료
> 라이브러리: **Adafruit NeoPixel** + **PubSubClient** (라이브러리 관리에서 설치)

4단계(구글시트 폴링, 1~3초 지연)를 **MQTT**로 바꿔 버튼·음성 모두 **0.1~0.5초** 안에 반응합니다.
센서·자동·색·밝기 코드는 3·4단계와 같고 통신만 다릅니다. 브로커 만들기 → [`../../../docs/mqtt.md`](../../../docs/mqtt.md)

## 업로드 전에 바꿀 것

```cpp
const char* WIFI_SSID    = "와이파이이름";       // 2.4GHz만
const char* WIFI_PASS    = "와이파이비밀번호";
const char* MQTT_HOST    = "xxxxxxxx.ala.asia-southeast1.emqxsl.com";   // 선생님이 준 브로커 주소
const int   MQTT_PORT    = 8883;                 // TLS
const char* MQTT_USER    = "moodlamp";           // 브로커 계정 (공개 브로커면 "")
const char* MQTT_PASS    = "비밀번호";
const char* TOPIC_PREFIX = "moodlamp/3반-a7x9";  // 선생님이 준 우리 반 문자열
const char* TEAM_ID      = "TEAM01";             // 내 팀 번호 — 12팀이 모두 달라야 해요

const int  DARK_LEVEL   = 30;      // 1단계에서 정한 숫자
const bool DARK_IS_HIGH = true;    // 1단계에서 확인한 방향
```

## 동작
- 브로커의 `TOPIC_PREFIX/TEAM_ID/cmd`를 구독하고, 앱이 보낸 한 줄 명령을 **즉시** 실행 — `ON` `OFF` `AUTO` `COLOR blue` `COLOR FF8800` `BRIGHT 50` `CONFIG 30 300`
- 상태가 바뀌면 바로, 아니어도 30초마다 `…/state`에 JSON 보고 (retained) · `…/status`에 `online`, 끊기면 브로커가 `offline`
- 통신은 다른 코어 — 와이파이·브로커가 끊겨도 자동 동작·페이드는 계속, 5초마다 재연결
- `CONFIG`(어두움 기준 %·자동 꺼짐 초)는 보드에 저장돼 껐다 켜도 유지

## 시리얼 모니터 (115200)
`on` · `off` · `auto` · `s`(상태 + 브로커 연결 상태) · `c red` / `c FF8800` · `b 50`
연결 실패 시 원인(-2 주소·포트, 5 인증 실패 …)을 함께 찍어 줍니다.

## 확인
- [ ] 시리얼에 "브로커 연결됨 — 앱 명령 대기 (moodlamp/…/TEAM01/cmd)"
- [ ] 브로커 웹 클라이언트에서 `TOPIC_PREFIX/#` 구독 → `TEAM01/status` = online, `TEAM01/state` JSON 보임
- [ ] 웹 클라이언트에서 `TOPIC_PREFIX/TEAM01/cmd`에 `ON` 발행 → **즉시** 켜짐
- [ ] 앱 '켜기' · 🎤 "켜 줘" → 1초 안에 켜짐
- [ ] 보드 전원을 빼면 30초 안에 status가 offline으로 바뀜
