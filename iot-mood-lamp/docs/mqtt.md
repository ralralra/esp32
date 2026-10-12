# MQTT 브로커 — 폰에서 누르면 바로 켜지게 (5단계)

4단계(구글시트 + Apps Script)는 보드가 2초마다 "새 명령 있나요?" 하고 묻는 구조라 **1~3초 늦게** 반응합니다.
5단계는 **MQTT 브로커**를 가운데 두고, 앱이 보낸 명령을 브로커가 보드에 **곧바로 밀어주어** 0.1~0.5초 안에 반응합니다.
버튼도, 음성도 같은 길을 쓰므로 둘 다 빨라집니다.

```
[폰 앱 (브라우저)] ──wss://──▶ ┌─────────────┐ ──mqtts://──▶ [보드 ×12]
   publish …/TEAMnn/cmd       │  MQTT 브로커  │   subscribe …/TEAMnn/cmd
   subscribe …/+/state        │ (EMQX Cloud)  │   publish   …/TEAMnn/state (retained)
                              └─────────────┘   publish   …/TEAMnn/status online/offline
```

- 4단계 구글시트는 그대로 두고, **통신 통로만** 바꿉니다. 명령 이름(`ON`·`COLOR`…)과 팀 구조는 같습니다.
- 브로커는 **선생님이 한 번** 만들고, 주소·계정을 학생에게 나눠 줍니다.

## 브로커 고르기

| | ① EMQX Cloud Serverless (권장) | ② 공개 브로커 `broker.emqx.io` |
|---|---|---|
| 계정 | 선생님 가입 1회 (무료) | 없음 |
| 비용 | 월 무료 할당: 세션 100만 분 · 트래픽 1GB (12대 한 달 켜 둬도 약 52만 분) | 무료 |
| 보안 | **계정·비밀번호** 필요 — 우리 반만 접속 | **누구나** 접속 가능 → `TOPIC_PREFIX`를 긴 임의 문자열로 |
| 포트 | TLS 8883 (보드) · WSS 8084 (앱) — TLS 없는 1883·8083은 지원 안 함 | 1883 · TLS 8883 · WS 8083 · WSS 8084 |
| 용도 | 수업·시연 | 계정 만들기 전 빠른 테스트 |

> HiveMQ Cloud의 무료(Serverless) 요금제는 2026-12-31에 종료된다고 공지되어 있어 권하지 않습니다.

## ① EMQX Cloud Serverless 만들기 (선생님, 10분)

1. [cloud.emqx.com](https://www.emqx.com/en/cloud) 가입 → **Create Deployment → Serverless** (무료 할당량 안에서 $0)
2. 배포가 뜨면 **Overview**에서 주소 확인 → `xxxxxxxx.ala.asia-southeast1.emqxsl.com` 같은 형식
3. **Access Control → Authentication → Add** 로 계정 하나 만들기 (예: `moodlamp` / 비밀번호). 반 전체가 같은 계정을 씁니다.
4. 학생에게 나눠 줄 것 — **주소 · 계정 · 비밀번호 · 우리 반 TOPIC_PREFIX**(예: `moodlamp/3반-a7x9`)
5. 확인: 배포 페이지의 **Online MQTT Client**(또는 [MQTTX 웹](https://mqttx.app/web-client))에서 `moodlamp/3반-a7x9/#` 를 구독해 두고
   보드를 켜면 `…/TEAM01/status` = `online` 과 `…/TEAM01/state` JSON이 보여야 합니다.

## ② 공개 브로커로 바로 테스트

펌웨어에서 이렇게만 바꾸면 계정 없이 됩니다. **교실 밖 누구나 같은 토픽을 쓸 수 있으니** `TOPIC_PREFIX`를 추측 못 할 문자열로 하고, 수업이 끝나면 ①로 옮기세요.

```cpp
const char* MQTT_HOST    = "broker.emqx.io";
const int   MQTT_PORT    = 8883;          // TLS (1883 + MQTT_TLS=false 도 됨)
const char* MQTT_USER    = "";
const char* MQTT_PASS    = "";
const char* TOPIC_PREFIX = "moodlamp/k7q2-x9z1-3ban";
```

앱은 `wss://broker.emqx.io:8084/mqtt` 로 접속합니다.

## 토픽 약속 (앱 · 보드 공통)

`TOPIC_PREFIX/TEAMnn/` 아래 세 개입니다.

| 토픽 | 방향 | 내용 |
|---|---|---|
| `…/cmd` | 앱 → 보드 | 한 줄 글자: `ON` · `OFF` · `AUTO` · `COLOR blue` · `COLOR FF8800` · `BRIGHT 50` · `CONFIG 30 300` |
| `…/state` | 보드 → 앱 (retained) | `{"type":"neopixel","power":"ON","auto":1,"color":"warm","bright":60,"light":32,"motion":1,"dark":30,"off":300}` (전구형은 `type:"bulb"`, color·bright 없음) |
| `…/status` | 보드 → 앱 (retained) | `online` / `offline` — 보드가 끊기면 브로커가 유언(LWT)으로 `offline`을 대신 남김 |

- **retained**: 브로커가 마지막 값을 기억해 두어, 앱이 나중에 접속해도 현재 상태가 바로 보입니다.
- 보드는 상태가 바뀌면 바로, 아니어도 30초마다 `state`를 보냅니다. 움직임 변화는 5초에 한 번까지.
- 앱의 "반 전체 연결 현황"은 `TOPIC_PREFIX/+/status` 를 구독해서 만듭니다.
- 4단계 명령 이름과 같아서, 음성 규칙([`voice_control.md`](voice_control.md))은 그대로 씁니다.

## 12팀이면 얼마나 쓰나

| 항목 | 값 |
|---|---|
| 연결 수 | 보드 12 + 폰 12 = 24 |
| 메시지 | 명령은 누를 때만, 상태는 30초마다 약 150바이트 → 12대 하루 약 5MB |
| 세션 분 (①) | 12대 × 24시간 × 30일 ≈ 52만 분 (무료 100만 분의 절반) |

반응이 느리거나 끊기면 거의 다 **교실 와이파이** 문제입니다 (2.4GHz인지, 로그인 페이지가 뜨는 종류인지, 8883 포트가 막혀 있지 않은지).

## 펌웨어·앱

| | V1 네오픽셀 | V2 릴레이 전구 |
|---|---|---|
| 펌웨어 | [`step5_app_mqtt`](../v1_neopixel/firmware/step5_app_mqtt/) | [`step5_app_mqtt`](../v2_relay_bulb/firmware/step5_app_mqtt/) |
| 라이브러리 | Adafruit NeoPixel + **PubSubClient** | **PubSubClient** |
| 앱 프롬프트 | [`app/ai_studio_prompt_mqtt.md`](../app/ai_studio_prompt_mqtt.md) (공통) | |
