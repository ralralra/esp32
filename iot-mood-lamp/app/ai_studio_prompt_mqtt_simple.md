# AI Studio 앱 프롬프트 — 간단판 (MQTT · 디자인 자유)

디자인 지시 없이 **연결 값 · 기능 · 통신 규칙**만 적은 프롬프트입니다. 학생은 **맨 위 `내 팀` 한 줄만** 바꾸고, 화면 모양은 마음대로 꾸밉니다.
자세한 화면 지시가 있는 판은 [`ai_studio_prompt_mqtt.md`](ai_studio_prompt_mqtt.md).

사용법: 아래 상자 안을 통째로 복사 → `TEAM01`을 내 팀 번호로 → [aistudio.google.com](https://aistudio.google.com) → Build → 붙여넣기 → 실행.
선생님: 배포 전에 `[비밀번호]`를 실제 값으로 채워서 나눠 주세요.

---

```
내 팀: TEAM01        ← 이 한 줄만 내 팀 번호로 바꾼다 (TEAM01 ~ TEAM12)

ESP32 무드등을 폰에서 제어하는 웹앱을 만들어 줘. 화면 디자인·색·배치·글꼴은 네가 보기 좋게 자유롭게 정해.
단, 아래 "기능"과 "통신 규칙"은 그대로 지켜야 보드가 반응해.

━━━━━━━━━━ 연결 (고정값) ━━━━━━━━━━
- MQTT over WebSocket. mqtt.js(브라우저용, unpkg의 mqtt.min.js)를 써.
- 주소: wss://if3426ad.ala.eu-central-1.emqxsl.com:8084/mqtt
- 계정: moodlamp   비밀번호: [비밀번호]
- clientId: "app-" + 내팀 + "-" + 랜덤 6글자,  reconnectPeriod 2000,  keepalive 30
- 토픽 앞부분: moodlamp/IoT
  · 명령 보내는 곳 (publish):  moodlamp/IoT/내팀/cmd
  · 상태 받는 곳 (subscribe):  moodlamp/IoT/내팀/state
  · 연결 여부 (subscribe):     moodlamp/IoT/내팀/status   ("online" / "offline")
- 설정 화면에 팀 선택(TEAM01~TEAM12)을 두고 처음 값은 위의 "내 팀". localStorage에 저장. 팀을 바꾸면 구독을 바꾸고 그 팀의 마지막 state로 화면을 갱신.

━━━━━━━━━━ 보드가 보내는 상태 (state, JSON, retained) ━━━━━━━━━━
예: {"type":"combo","power":"ON","np":"ON","bulb":"OFF","auto":1,"color":"warm","bright":60,"light":32,"motion":1,"dark":30,"off":60}
- type: "neopixel" = 네오픽셀만 · "bulb" = 전구만 · "combo" = 네오픽셀 + 전구 둘 다. type에 따라 보여줄 조작을 바꿔.
- power ON/OFF: 켜짐 여부 (combo는 둘 중 하나라도 켜지면 ON)
- np, bulb ON/OFF: combo일 때만 옴 — 네오픽셀·전구 각각의 상태
- auto 1/0: 자동 모드 / 수동 모드
- color: warm·white·red·orange·yellow·green·blue·purple·pink·rainbow 또는 6자리 색 코드  (bulb에는 없음)
- bright 0~100: 네오픽셀 밝기 (bulb에는 없음)
- light 0~100: 주변 밝기 %,  motion 1/0: 사람 감지
- dark: 어두움 기준 %,  off: 초 (neopixel·bulb는 '사람 없으면 꺼지는 시간', combo는 '네오픽셀 유지 시간')
state는 retained라 접속하자마자 마지막 값이 온다. 아직 한 번도 못 받았으면 "보드를 켜고 와이파이에 연결해 주세요"라고 보여줘.
헤더의 "연결됨" 표시는 (앱이 브로커에 붙음) AND (내 팀 status == "online") 일 때만.

━━━━━━━━━━ 앱이 보내는 명령 (cmd, 글자 그대로, retain 끔, qos 0) ━━━━━━━━━━
- "ON" / "OFF"            켜기 / 끄기 (combo는 둘 다)
- "NP ON" / "NP OFF"      combo: 네오픽셀만
- "BULB ON" / "BULB OFF"  combo: 전구만
- "AUTO"                  자동 모드로
- "MANUAL"                combo에서 수동으로 바꿀 때 (지금 켜진 것은 그대로 두고 자동만 끔)
                          neopixel·bulb에서 수동으로 바꿀 때는 지금 켜져 있으면 "ON", 꺼져 있으면 "OFF"
- "COLOR blue" / "COLOR FF8800"   네오픽셀 색 (프리셋 이름 또는 # 없는 6자리 색 코드)
- "BRIGHT 50"             네오픽셀 밝기 0~100 — 슬라이더는 손을 뗄 때 한 번만 보내
- "CONFIG 30 60"          어두움 기준(0~95) 한 칸, 초(30/60/300/600) 한 칸 — 자동화 설정 저장 버튼
JSON으로 감싸지 말고 위 글자 그대로 보내. 버튼을 누르면 화면을 먼저 바꾸고(낙관적 갱신), 다음 state가 오면 실제 값으로 맞춰. 2초 안에 state가 안 오면 "응답 없음" 표시.

━━━━━━━━━━ 꼭 있어야 하는 기능 ━━━━━━━━━━
1. 조명 탭
   - neopixel: 켜기/끄기 버튼, [수동|자동] 전환, 색 고르기(프리셋 4개 이상 + 컬러휠 또는 색 입력), 밝기 슬라이더
   - bulb: 켜기/끄기 버튼, [수동|자동] 전환 (색·밝기 조작은 숨김)
   - combo: 네오픽셀 스위치, 전구 스위치, 모두 켜기/끄기, [수동|자동] 전환, 색·밝기(네오픽셀용)
   - 공통: 주변 밝기 %, 사람 감지 여부 표시, 큰 "눌러서 말하기" 버튼
2. 자동화 탭: 어두움 기준 슬라이더(0~95), 시간 드롭다운(30초/1분/5분/10분), 처음 값은 state의 dark·off, "설정 저장" → CONFIG
3. 설정 탭: 팀 선택, 연결 상태, 브로커 주소 표시
4. 음성 명령: Web Speech API, lang 'ko-KR'. 한 문장을 듣고 아래 순서로 판정해서 명령 하나를 보내.
   ① '자동' 포함 → "AUTO" 하고 끝 ("자동 모드 켜 줘"도 AUTO)
   ② combo면 대상 먼저: '네오픽셀'·'무드등'·'LED' → NP, '전구'·'램프' → BULB, 없으면 둘 다
   ③ '꺼' 포함 → 대상에 맞게 "NP OFF" / "BULB OFF" / "OFF",  '켜' 포함 → "NP ON" / "BULB ON" / "ON"  ('꺼'를 먼저 검사)
   ④ 색 단어 → "COLOR …" (빨강 red, 주황 orange, 노랑 yellow, 초록 green, 파랑·파란 blue, 보라 purple, 분홍 pink, 따뜻 warm, 하양·흰 white, 무지개 rainbow)
   ⑤ '밝기'+숫자 → "BRIGHT 숫자", '밝게' → 현재+20, '어둡게' → 현재−20 (0~100)
   - bulb 보드에서 색·밝기가 들리면 보내지 말고 "전구는 켜기/끄기만 돼요" 안내. 못 알아들으면 "다시 말해 주세요".
   - 음성 인식이 없는 브라우저면 마이크 버튼을 비활성화하고 "안드로이드는 Chrome, 아이폰은 Safari" 안내.
5. 연결이 끊기면 헤더에 "연결 끊김", 명령 버튼 비활성화, mqtt.js의 자동 재접속에 맡겨.

━━━━━━━━━━ 하지 말 것 ━━━━━━━━━━
- cmd를 retain으로 보내지 않기 (보드가 재부팅할 때마다 그 명령을 다시 받게 됨)
- 토픽에 # 이나 + 를 넣어 publish 하지 않기
- 상태를 주기적으로 물어보는 폴링 만들지 않기 — state는 보드가 바뀔 때와 30초마다 알아서 보냄
```

---

## 안 될 때

| 증상 | 확인 |
|---|---|
| 헤더가 "연결 끊김" | 주소가 `wss://…:8084/mqtt`인지, 계정·비밀번호, 폰이 인터넷 되는지 |
| 연결은 됐는데 "보드를 켜고…"만 나옴 | 보드 시리얼에 "브로커 연결됨"이 떴는지, 보드와 앱의 팀 번호가 같은지, 토픽 앞부분이 `moodlamp/IoT`인지 |
| 버튼을 눌러도 안 켜짐 | 앱이 보낸 글자가 `ON`처럼 그대로인지(JSON 아님), 다른 팀 토픽으로 보내고 있지 않은지 |
| 다른 팀 램프가 같이 움직임 | 팀 번호가 겹침 — 설정 탭에서 내 팀으로 |
