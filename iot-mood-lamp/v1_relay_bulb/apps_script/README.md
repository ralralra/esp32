# V1 중계 서버 — 명령·상태 이름 약속 (초안)

> ⏳ 준비 중 — `lamp_relay.gs` 는 이 폴더에 추가 예정
> 이 표가 **앱 · Apps Script · ESP32 세 곳의 공통 약속**입니다. 철자가 하나라도 다르면 동작하지 않아요.

## 앱 → 서버 (명령 보내기)

| 요청 | 의미 |
|---|---|
| `?mode=set&cmd=ON` | 전구 켜기 (수동 모드로 전환) |
| `?mode=set&cmd=OFF` | 전구 끄기 (수동 모드로 전환) |
| `?mode=set&cmd=AUTO` | 자동 모드로 복귀 |

## 앱 ← 서버 (상태 읽기)

`?mode=state` →
```json
{ "power": "ON", "auto": 1, "light": 1234, "motion": 1, "updated": "2026-10-04 19:30:12" }
```

## ESP32 ↔ 서버

| 요청 | 응답 / 의미 |
|---|---|
| `?mode=next` | 처리 안 한 가장 오래된 명령 1개 → `{"cmd":"ON","value":""}` / 없으면 `{"cmd":""}` |
| `?mode=report&power=ON&auto=1&light=1234&motion=1` | 현재 상태 덮어쓰기 |

- `mode` 는 **요청 종류**(set·state·next·report)에만 씁니다. 램프의 자동/수동은 `auto`(1=자동, 0=수동)로 따로 표시해 이름이 겹치지 않게 했습니다.
- `light` 는 조도 원시값(0~4095), `motion` 은 인체감지(1=감지)입니다.

## 시트 탭 (초안)

| 탭 | 제목 행 |
|---|---|
| `commands` | 시각 · cmd · value · 처리됨 |
| `state` | power · auto · light · motion · updated (1행만 계속 덮어씀) |
| `log` (선택) | 시각 · 이벤트 — **인체감지 기록을 남길지는 팀에서 먼저 결정** |
