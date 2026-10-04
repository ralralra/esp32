# 음성제어 — 폰으로 "무드등 켜"

결론: **가능합니다.** 가장 확실한 방법은 AI Studio 앱 안에 **마이크 버튼**을 넣는 것입니다.

## 방법 1 (권장) — Web Speech API 마이크 버튼

브라우저에 내장된 음성 인식으로 말을 글자로 바꾸고, 정해 둔 단어가 들어 있으면 명령을 보냅니다.

| 들린 말에 포함된 단어 | 보낼 명령 (→ [V1](../v1_relay_bulb/apps_script/README.md) · [V2](../v2_neopixel_color/apps_script/README.md) 명령 약속) |
|---|---|
| 켜 | `cmd=ON` |
| 꺼 | `cmd=OFF` |
| 빨강 / 노랑 / 파랑 / 따뜻 … (V2) | `cmd=COLOR&value=…` |
| 밝게 / 어둡게 (V2) | `cmd=BRIGHT&value=…` (현재값 ± 단계) |
| 자동 | `cmd=AUTO` |

AI Studio 지시문 예시:

```
화면에 큰 마이크 버튼을 추가해줘.
누르면 Web Speech API(lang='ko-KR')로 한 문장을 듣고,
문장에 '켜'가 있으면 [웹앱 URL]?mode=set&cmd=ON,
'꺼'가 있으면 [웹앱 URL]?mode=set&cmd=OFF 로 요청해줘.
들은 문장은 화면에 그대로 보여줘.
```

### 꼭 확인할 것

- **지원 브라우저: 안드로이드 Chrome, 아이폰 Safari(iOS 14.5 이상).** 시연은 이 둘로.
- 음성은 구글·애플 서버에서 인식 → **인터넷 연결 필수.**
- AI Studio 앱에서 마이크를 쓰려면 **`metadata.json` 의 `requestFramePermissions` 에 `"microphone"` 추가.**
  사용자에게는 마이크 사용 동의 화면이 한 번 더 뜹니다.
  ([AI Studio Build 문서](https://ai.google.dev/gemini-api/docs/aistudio-build-mode))
- '켜'와 '꺼'를 같이 말하는 문장("켜지 말고 꺼")처럼 단어 매칭이 헷갈리는 경우를 테스트 시나리오에 넣어 두세요.
- 아이폰 **홈 화면에 추가한 앱** 상태에서의 마이크 동작은 확인하지 못했습니다 →
  Safari에서 직접 연 경우와 홈 화면 앱 두 가지 모두 시연 전에 시험.

## 방법 2 (심화) — Gemini로 자연스러운 말 알아듣기

"좀 따뜻하고 은은하게 해줘"처럼 정해진 단어가 없는 문장을 Gemini에 보내
`{"cmd":"COLOR","value":"warm"}`, `{"cmd":"BRIGHT","value":40}` 같은 명령으로 바꿉니다.

- 방법 1이 완성된 뒤에 **단어 매칭 실패 시에만** Gemini에 넘기는 식으로 얹으면 됩니다.
- 공유 링크로 다른 사람이 쓸 때의 API 사용량 처리는 AI Studio 정책을 확인해야 합니다 (바뀔 수 있음).

## 보너스 — 아이폰 Siri 단축어

iOS **단축어** 앱 → 새 단축어 → **"URL의 콘텐츠 가져오기"** 에
`[웹앱 URL]?mode=set&cmd=ON` 을 넣고 이름을 "무드등 켜"로 저장하면
**"시리야, 무드등 켜"** 로 동작시킬 수 있습니다. (시연 전 실제 동작 확인 필요)
