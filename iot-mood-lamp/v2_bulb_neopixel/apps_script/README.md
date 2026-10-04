# V2 중계 서버 — 명령·상태 이름 약속 (초안)

> ⏳ 준비 중 — `mood_lamp_relay.gs` 는 이 폴더에 추가 예정
> [V1 약속](../../v1_relay_bulb/apps_script/README.md)을 **그대로 유지하고 아래만 추가**합니다.
> 그래서 V1용 앱·음성 명령(`ON` · `OFF` · `AUTO`)은 V2에서도 그대로 동작합니다.

## 앱 → 서버 (명령 보내기)

| 요청 | 의미 | |
|---|---|:---:|
| `?mode=set&cmd=ON` / `OFF` | 전구 + 네오픽셀 **함께** 켜기/끄기 (수동 모드로 전환) | V1 |
| `?mode=set&cmd=AUTO` | 자동 모드로 복귀 | V1 |
| `?mode=set&cmd=BULB&value=ON` / `OFF` | 전구만 | **추가** |
| `?mode=set&cmd=LED&value=ON` / `OFF` | 네오픽셀만 | **추가** |
| `?mode=set&cmd=COLOR&value=warm` | 색 — 이름: `warm` `white` `red` `orange` `yellow` `green` `blue` `purple` `pink` | **추가** |
| `?mode=set&cmd=COLOR&value=FF8800` | 색 — 6자리 색 코드 (`#` 없이 — URL에서 `#`은 잘림) | **추가** |
| `?mode=set&cmd=BRIGHT&value=60` | 네오픽셀 밝기 0~100 (%) | **추가** |

## 앱 ← 서버 (상태 읽기)

`?mode=state` →
```json
{ "bulb": "ON", "led": "ON", "color": "warm", "bright": 60,
  "auto": 1, "light": 1234, "motion": 1, "updated": "2026-10-04 19:30:12" }
```

V1의 `power` 는 V2에서 `bulb`(전구)와 `led`(네오픽셀)로 나뉩니다.

## ESP32 ↔ 서버

| 요청 | 응답 / 의미 |
|---|---|
| `?mode=next` | `{"cmd":"COLOR","value":"warm"}` / 없으면 `{"cmd":""}` (V1과 같음) |
| `?mode=report&bulb=ON&led=ON&color=warm&bright=60&auto=1&light=1234&motion=1` | 현재 상태 덮어쓰기 |

## 시트 탭 (초안)

| 탭 | 제목 행 |
|---|---|
| `commands` | 시각 · cmd · value · 처리됨 (V1과 같음) |
| `state` | bulb · led · color · bright · auto · light · motion · updated |
| `log` (선택) | 시각 · 이벤트 — **인체감지 기록을 남길지는 팀에서 먼저 결정** |
