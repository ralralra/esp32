# V1 앱 — AI Studio 프롬프트

> ⏳ 준비 중 — 완성 프롬프트(`ai_studio_prompt.md`)는 이 폴더에 추가 예정

## 화면 구성 (초안)
- 큰 전구 아이콘 + 현재 상태(켜짐/꺼짐 · 자동/수동)
- 버튼 3개: **켜기 · 끄기 · 자동**
- 🎤 마이크 버튼 — "켜" / "꺼" / "자동" → [`../../docs/voice_control.md`](../../docs/voice_control.md)
- 작은 카드: 주변 밝기 · 사람 감지 여부

## 연결
- 명령·상태 주소는 [`../apps_script/README.md`](../apps_script/README.md) 의 약속을 그대로 사용
- 마이크 사용 시 `metadata.json` 의 `requestFramePermissions` 에 `"microphone"` 추가
- 프롬프트 작성 요령은 수업 가이드 [`ai_studio_webapp_guide.md`](../../../esp32-class-projects/01_docs/ai_studio_webapp_guide.md) 참고
