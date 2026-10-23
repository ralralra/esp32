# V3 · 5단계 — 앱·음성 연동 '빠른 응답' 펌웨어 (네오픽셀 + 전구 · MQTT)

> ✅ 코드: [`step5_app_mqtt.ino`](step5_app_mqtt.ino) — 동작 검증·ESP32 컴파일 확인 완료
> 라이브러리: **Adafruit NeoPixel** + **PubSubClient**

3단계의 자동 동작(사람 → 네오픽셀, 어두움 → 전구)에 MQTT를 얹어, 앱 버튼과 음성이 **0.1~0.5초** 안에 반응합니다.
V1·V2 5단계와 같은 브로커·앱을 쓰고, 조명이 둘이라 명령과 상태 항목이 조금 늘어납니다. 브로커 만들기 → [`../../../docs/mqtt.md`](../../../docs/mqtt.md)

## 업로드 전에 바꿀 것

```cpp
const char* WIFI_SSID    = "와이파이이름";       // 2.4GHz만
const char* WIFI_PASS    = "와이파이비밀번호";
const char* MQTT_HOST    = "xxxxxxxx.ala.asia-southeast1.emqxsl.com";   // 선생님이 준 브로커 주소
const int   MQTT_PORT    = 8883;
const char* MQTT_USER    = "moodlamp";
const char* MQTT_PASS    = "비밀번호";
const char* TOPIC_PREFIX = "moodlamp/3반-a7x9";  // 선생님이 준 우리 반 문자열
const char* TEAM_ID      = "TEAM01";             // 내 팀 번호

const int  DARK_LEVEL   = 30;      // 1단계에서 정한 숫자
const bool DARK_IS_HIGH = true;    // 1단계에서 확인한 방향
const int  MAX_BRIGHTNESS = 128;   // 전원 B·C안 기준. 9V 건전지 A안이면 64
```

**전원**: 와이파이를 켜면 ESP32가 0.24A를 더 쓰므로 9V 건전지(A안)는 권하지 않습니다. B안(벅 컨버터)·C안(220V→5V 모듈)이나 5V 2A 충전기 → [`../../wiring.md`](../../wiring.md)

## 명령 (앱 → `…/cmd`)

| 명령 | 동작 |
|---|---|
| `ON` / `OFF` | 둘 다 켜기 / 끄기 (수동 모드로) |
| `NP ON` / `NP OFF` | 네오픽셀만 |
| `BULB ON` / `BULB OFF` | 전구만 |
| `AUTO` | 자동 모드 — 사람 → 네오픽셀, 어두움 → 전구 |
| `MANUAL` | 자동만 끔, 지금 켜진 것은 그대로 (앱 [수동] 버튼) |
| `COLOR blue` · `COLOR FF8800` | 네오픽셀 색 |
| `BRIGHT 50` | 네오픽셀 밝기 % |
| `CONFIG 30 60` | 어두움 기준 % · 사람 없을 때 네오픽셀 유지 초 (보드에 저장) |

## 상태 (보드 → `…/state`, retained)

```json
{"type":"combo","power":"ON","np":"ON","bulb":"OFF","auto":1,"color":"warm","bright":60,"light":32,"motion":1,"dark":30,"off":60}
```

- `type` 이 `combo` 이면 앱은 네오픽셀·전구 스위치를 둘 다 보여줍니다 (앱 프롬프트의 화면 4).
- `power` 는 둘 중 하나라도 켜져 있으면 ON — V1·V2용 앱으로 봐도 켜짐·꺼짐은 맞게 보입니다.
- `off` 는 V1·V2의 '자동 꺼짐 초'가 아니라 **네오픽셀 유지 초**입니다.

## 시리얼 모니터 (115200)
`on` · `off` · `np on|off` · `bulb on|off` · `auto` · `manual` · `s`(상태 + 브로커 연결 상태) · `c <색>` · `b <0~100>`

## 확인
- [ ] 시리얼에 "브로커 연결됨 — 앱 명령 대기 (moodlamp/…/TEAM01/cmd)"
- [ ] 브로커 웹 클라이언트에서 `…/TEAM01/cmd` 에 `NP ON` → 네오픽셀만, `BULB ON` → 전구만 **즉시**
- [ ] 앱에서 네오픽셀·전구 스위치 각각, 🎤 "네오픽셀 켜 줘" · "전구 꺼 줘" · "파란색으로" → 1초 안에 반응
- [ ] `AUTO` 뒤 밝은 곳에서 손 흔들기 → 네오픽셀만, 불 끄기 → 전구 켜짐
- [ ] 보드 전원을 빼면 30초 안에 status가 offline으로 바뀜
