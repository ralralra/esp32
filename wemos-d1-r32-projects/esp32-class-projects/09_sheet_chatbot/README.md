# 🤖 심화 — 구글 시트 기록을 답해주는 AI 챗봇 (Function Calling)

> 4회차에서 만든 **교실 환경 모니터**(ESP32 → Apps Script → 구글 시트)에 AI를 붙입니다.
> "지금 교실 온도 몇 도야?", "오늘 최고 소음은?" 하고 물으면 **시트의 실제 데이터를 읽고 대답하는** 챗봇을 만들어요.
>
> **선수 조건**: [4회차 클라우드 확장](../02_sessions/04_cloud_expansion/README.md) 완료 — 시트에 데이터가 쌓이고 있어야 합니다.

## 완성하면 이런 구조

```
📡 ESP32 + 센서 ──(10초마다 업로드)──▶ 📊 Google Sheets
                                            ▲
                                            │ 파이썬이 시트를 읽음
                                            ▼
💬 사용자 "교실 온도 어때?" ──▶ 🧠 GPT ──▶ 🐍 파이썬 함수 실행 ──▶ 💬 "현재 26도예요!"
                                  (gradio 채팅 화면)
```

ESP32 쪽은 **하나도 안 바꿔도 됩니다.** 시트에 쌓인 데이터를 읽는 파이썬 프로그램을 새로 만드는 거예요.

## 준비물

- [ ] 4회차 시트 — 1행 제목 `시각 | 온도 | 습도 | 조도 | 소음`, 데이터가 쌓이는 중
- [ ] **파이썬 3.9 이상** — 노트북에 없다면 [python.org](https://www.python.org/downloads/)에서 설치 (설치 시 "Add Python to PATH" 체크!)
- [ ] **OpenAI API 키** — [platform.openai.com/api-keys](https://platform.openai.com/api-keys)에서 발급
  - ⚠️ **키는 코드·깃허브·단톡방에 절대 올리지 않기!** 유출되면 남이 내 돈으로 GPT를 씁니다.
- [ ] 패키지 설치 (명령 프롬프트/터미널에서):

```bash
pip install openai gradio pandas
```

---

# 1교시 — 파이썬으로 시트 읽어오기

## ① 시트를 "링크 공유"로 열기

시트 오른쪽 위 **공유** → 일반 액세스: **링크가 있는 모든 사용자 — 뷰어** → 완료.
그리고 주소창에서 **시트 ID**를 복사해둡니다:

```
https://docs.google.com/spreadsheets/d/【이 부분이 시트 ID】/edit#gid=0
```

## ② 파이썬 3줄로 시트 전체 읽기

새 파일 `sheet_test.py`:

```python
import pandas as pd

SHEET_ID = "여기에_시트_ID_붙여넣기"
url = f"https://docs.google.com/spreadsheets/d/{SHEET_ID}/export?format=csv"

df = pd.read_csv(url)
print(df.tail())        # 마지막 5줄 출력
```

실행해서 시트의 최근 데이터가 표로 나오면 성공! 🎉

```
        시각    온도  습도  조도  소음
95  14:20:11   26   61   512   43
96  14:20:21   26   61   508   45
...
```

> 💡 **원리**: 구글 시트는 `/export?format=csv` 주소로 요청하면 시트 전체를 CSV(쉼표로 구분된 텍스트)로 보내줍니다.
> `pandas`가 그걸 받아서 표(DataFrame)로 만들어줘요. Apps Script를 거치지 않는 **읽기 전용 지름길**입니다.

---

# 2교시 — Function Calling 제대로 이해하기

## 왜 필요할까? — GPT는 우리 시트를 볼 수 없다

GPT에게 그냥 "교실 온도 몇 도야?"라고 물으면 이렇게 답합니다:

> "죄송하지만 저는 실시간 교실 온도를 알 수 없습니다."

당연해요 — GPT는 인터넷 너머의 **우리 시트에 접근할 방법이 없으니까요.**
**Function Calling은 GPT에게 "우리가 만든 함수"를 손발로 빌려주는 기능**입니다.
함수 목록을 미리 알려주면, GPT가 필요할 때 "이 함수 좀 실행해줘"라고 요청해요.

## 🔑 가장 중요한 개념: GPT는 함수를 직접 실행하지 않는다!

많은 사람이 오해하는 부분입니다. 실제 흐름을 대화로 표현하면:

```
우리:   GPT야, 'get_latest_reading'이라는 함수가 있어.
        교실의 최신 센서값(온도·습도·조도·소음)을 돌려주는 함수야.
GPT:    네, 기억해둘게요.

사용자:  지금 교실 온도 몇 도야?
GPT:    (스스로 판단) 이건 함수가 필요한 질문이네!
        → "get_latest_reading 함수를 실행해주세요"          ← 요청만 함!
우리:   (파이썬이 함수 실행, 시트에서 읽음) 결과: 온도 26, 습도 61, ...
GPT:    (결과를 받아서) "지금 교실 온도는 26도예요!"          ← 결과로 답변 작성
```

| 역할 | 누가? |
|---|---|
| 어떤 함수를 쓸지 판단 | GPT |
| 함수를 **실제로 실행** | **우리 파이썬 코드** |
| 실행 결과로 자연스러운 답변 작성 | GPT |

GPT는 **판단**과 **말하기**만 하고, **행동은 전부 우리 코드**가 합니다.
그래서 안전해요 — GPT가 마음대로 우리 시트를 지우거나 할 수 없습니다. 우리가 만들어준 함수만, 우리 코드를 통해서만 실행됩니다.

## 전체 5단계

| 단계 | 하는 일 | 누가 |
|:---:|---|:---:|
| 1 | 파이썬 함수 만들기 | 우리 |
| 2 | 함수 "설명서"를 정해진 형식으로 작성 | 우리 |
| 3 | 질문 + 설명서를 GPT에 전달 → "어떤 함수를 어떤 값으로 부를지" 응답받기 | GPT |
| 4 | 그 함수를 실제로 실행 → 결과를 대화 기록에 추가 | **우리** |
| 5 | 결과가 포함된 대화를 다시 GPT에 전달 → 최종 답변 받기 | GPT |

---

# 3교시 — 5단계 직접 만들기

## Step 1 — 함수 2개 정의

시트를 읽는 함수를 **2개** 만듭니다. (2개인 이유: GPT가 질문에 따라 **골라 쓰는** 걸 보기 위해!)

```python
import pandas as pd

SHEET_ID = "여기에_시트_ID"
CSV_URL = f"https://docs.google.com/spreadsheets/d/{SHEET_ID}/export?format=csv"

def get_latest_reading():
    """최신 센서값 한 줄 (지금 상태)"""
    df = pd.read_csv(CSV_URL)
    last = df.iloc[-1]                      # 마지막 행
    return str({
        "시각": last["시각"], "온도": last["온도"], "습도": last["습도"],
        "조도": last["조도"], "소음": last["소음"],
    })

def get_sensor_stats(sensor):
    """특정 센서의 오늘 통계 (최저/최고/평균)"""
    df = pd.read_csv(CSV_URL)
    col = df[sensor]
    return str({
        "센서": sensor,
        "최저": round(col.min(), 1), "최고": round(col.max(), 1),
        "평균": round(col.mean(), 1), "기록수": len(col),
    })
```

- `get_latest_reading` — 인자 없음. "지금 어때?"용
- `get_sensor_stats` — 인자 1개(`sensor`). "오늘 최고 온도는?"용

## Step 2 — 함수 설명서 작성 (제일 중요!)

GPT는 파이썬 코드를 못 봅니다. **이 설명서만 보고** 함수를 언제, 어떻게 쓸지 판단해요.
그래서 `description`을 구체적으로 쓸수록 GPT가 똑똑하게 골라 씁니다.

```python
use_functions = [
    {
        "type": "function",
        "function": {
            "name": "get_latest_reading",
            "description": "교실의 가장 최근 센서 측정값(시각, 온도, 습도, 조도, 소음)을 가져온다. '지금', '현재' 상태를 물으면 이 함수를 쓴다.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "get_sensor_stats",
            "description": "특정 센서의 기록 전체에 대한 통계(최저, 최고, 평균, 기록 수)를 가져온다. '최고', '최저', '평균' 같은 질문에 이 함수를 쓴다.",
            "parameters": {
                "type": "object",
                "properties": {
                    "sensor": {
                        "type": "string",
                        "enum": ["온도", "습도", "조도", "소음"],   # 이 4개 중 하나만!
                        "description": "통계를 낼 센서 이름",
                    }
                },
                "required": ["sensor"],
            },
        },
    },
]
```

> 💡 `enum`은 "이 값들 중 하나만 골라라"는 제한입니다. GPT가 `temperature`처럼 엉뚱한 이름을 지어내는 걸 막아줘요.
> (시트 1행의 열 제목과 **정확히 같아야** 합니다!)

## Step 3 — 질문 + 설명서를 GPT에 전달

```python
import os, json
from openai import OpenAI

os.environ["OPENAI_API_KEY"] = "여기에_본인_키"      # ⚠️ 깃허브에 올리지 말 것!
client = OpenAI()

messages = [
    {"role": "system", "content": "당신은 교실 환경 모니터 도우미입니다. 제공된 함수로 센서 데이터를 조회해서 친절하게 답하세요."},
    {"role": "user", "content": "오늘 교실 최고 온도가 몇 도였어?"},
]

response = client.chat.completions.create(
    model="gpt-4o-mini",
    messages=messages,
    tools=use_functions,          # ← 설명서 전달!
)

response_message = response.choices[0].message
tool_calls = response_message.tool_calls
print(tool_calls)
```

출력을 보면 GPT의 "실행 요청"이 담겨 있습니다 (아직 실행 전!):

```
[... function=Function(arguments='{"sensor":"온도"}', name='get_sensor_stats') ...]
```

**"최고 온도"라는 질문을 보고 → `get_sensor_stats`를 → `sensor="온도"`로** 부르겠다고 스스로 판단한 거예요.

## Step 4 — 함수를 (우리가) 실행하고 결과를 기록

```python
available_functions = {
    "get_latest_reading": get_latest_reading,
    "get_sensor_stats": get_sensor_stats,
}

if tool_calls:
    messages.append(response_message)                       # GPT의 요청을 대화에 기록
    for tool_call in tool_calls:
        fn = available_functions[tool_call.function.name]   # 이름으로 함수 찾기
        args = json.loads(tool_call.function.arguments)     # 인자 꺼내기
        result = fn(**args)                                 # ★ 실행은 여기서, 우리가!
        messages.append({
            "tool_call_id": tool_call.id,
            "role": "tool",                                 # "함수 실행 결과"라는 표시
            "name": tool_call.function.name,
            "content": result,
        })
```

## Step 5 — 결과를 포함해 다시 GPT에게 → 최종 답변

```python
second = client.chat.completions.create(model="gpt-4o-mini", messages=messages)
print(second.choices[0].message.content)
# >> 오늘 교실의 최고 온도는 29.4도였어요! 평균은 26.2도로 기록 96건 기준입니다.
```

여기까지 실행되면 function calling의 전체 사이클을 한 바퀴 돈 것입니다. 👏

---

# 4교시 — gradio로 챗봇 화면 만들기

위 5단계를 함수 하나로 묶고, gradio 채팅 화면을 붙이면 완성입니다.
전체 코드: [`code/sheet_chatbot.py`](code/sheet_chatbot.py) — 시트 ID와 API 키만 바꾸면 바로 실행됩니다.

```bash
python sheet_chatbot.py
# Running on local URL: http://127.0.0.1:7860  ← 브라우저에서 열기
```

핵심 부분만 보면:

```python
import gradio as gr

def chat(message, history):
    # (5단계 사이클: 질문 → tool_calls → 실행 → 재질문 → 답변)
    return answer

gr.ChatInterface(
    fn=chat,
    title="🏫 교실 환경 챗봇",
    description="교실 온도·습도·조도·소음에 대해 물어보세요! (데이터: 우리 반 ESP32)",
).launch()
```

`gr.ChatInterface`는 함수 하나(`fn`)만 주면 **카카오톡 같은 채팅 화면**을 자동으로 만들어줍니다.

### 시험해볼 질문들

| 질문 | GPT가 고르는 함수 |
|---|---|
| "지금 교실 어때?" | `get_latest_reading()` |
| "오늘 제일 시끄러웠을 때 소음이 몇이야?" | `get_sensor_stats(sensor='소음')` |
| "습도 평균 알려줘" | `get_sensor_stats(sensor='습도')` |
| "안녕! 넌 누구야?" | 함수 안 씀 (그냥 대화) |

**함수를 쓸지 말지, 어떤 함수를 쓸지 GPT가 스스로 결정하는 것** — 이게 function calling의 핵심입니다.

---

## 🚀 도전과제 — 챗봇으로 ESP32 "제어"까지

4회차에서 명령 셀(A1)에 값을 쓰면 ESP32가 폴링으로 읽어가는 구조를 만들었죠?
**시트에 명령을 쓰는 함수**를 하나 추가하면, 챗봇이 읽기를 넘어 **제어**까지 하게 됩니다:

```
사용자: "교실 LED 켜줘"
GPT:   set_command(command="LED_ON") 실행 요청
파이썬: Apps Script 웹앱 주소로 요청 → 시트 A1에 "LED_ON" 기록
ESP32:  (10초 내 폴링) A1 읽음 → LED 켜짐!
```

힌트: 4회차의 Apps Script 웹앱 주소(`.../exec?mode=...`)를 `requests.get()`으로 호출하는 함수를 만들고,
설명서에 `enum: ["LED_ON", "LED_OFF"]`로 명령을 제한하세요. 말로 조종하는 IoT의 완성입니다! 🎛️

## 트러블슈팅

| 증상 | 원인/해결 |
|---|---|
| `pd.read_csv`에서 HTTP 오류 | 시트 공유가 "링크가 있는 모든 사용자"인지 확인, 시트 ID 복사 범위 확인 |
| 열 이름 KeyError (`'온도'`) | 시트 1행 제목과 코드의 열 이름이 **완전히** 같아야 함 (띄어쓰기 포함) |
| GPT가 이상한 센서 이름으로 호출 | 설명서의 `enum`에 열 이름 4개가 정확히 들어있는지 확인 |
| `AuthenticationError` | API 키 오타 또는 만료 — 새로 발급해서 교체 |
| 답변이 옛날 데이터 기준 | 함수가 호출될 때마다 `read_csv`를 새로 하는지 확인 (전역에서 한 번만 읽으면 갱신 안 됨) |
| 챗봇 화면이 안 열림 | 터미널의 `Running on local URL` 주소를 브라우저에 직접 입력 |
