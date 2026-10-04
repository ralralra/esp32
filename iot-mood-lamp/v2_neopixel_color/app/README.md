# V2 앱 — AI Studio 프롬프트

> ⏳ 준비 중 — 완성 프롬프트(`ai_studio_prompt.md`)는 이 폴더에 추가 예정

## 화면 구성 (초안) — V1 화면에 추가
- V1: 상태 표시 · **켜기 · 끄기 · 자동** 버튼 · 🎤 · 밝기/사람 감지 카드
- **추가**: 색상 동그라미 버튼 9개 (warm·white·red·orange·yellow·green·blue·purple·pink) + 직접 고르기
- **추가**: 밝기 슬라이더 0~100 — **손을 뗄 때(change 이벤트) 한 번만 전송**
- **추가**: 화면 배경이나 램프 그림이 현재 색으로 바뀜

## 음성 (→ [`../../docs/voice_control.md`](../../docs/voice_control.md))
- "켜" / "꺼" / "자동" — V1과 같음
- 색 이름("빨강", "따뜻하게" …) → `COLOR`
- "밝게" / "어둡게" → 현재 밝기 ±20 으로 `BRIGHT`

## 연결
- 명령·상태 주소는 [`../apps_script/README.md`](../apps_script/README.md) 의 약속을 그대로 사용
- 마이크 사용 시 `metadata.json` 의 `requestFramePermissions` 에 `"microphone"` 추가
