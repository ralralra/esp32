# 🏫 교실 환경 챗봇 — 구글 시트 기록을 Function Calling으로 답해주는 챗봇
#
# 준비: pip install openai gradio pandas
# 실행: python sheet_chatbot.py  →  브라우저에서 http://127.0.0.1:7860
#
# 아래 두 값만 본인 것으로 바꾸면 됩니다.

import os
import json
import pandas as pd
import gradio as gr
from openai import OpenAI

# ──────────────────────────────────────────────
# 설정 — 이 두 줄만 바꾸세요!
# ──────────────────────────────────────────────
SHEET_ID = "여기에_시트_ID"                      # 시트 주소의 /d/ 와 /edit 사이 문자열
os.environ["OPENAI_API_KEY"] = "여기에_본인_키"   # ⚠️ 깃허브·단톡방에 올리지 말 것!

CSV_URL = f"https://docs.google.com/spreadsheets/d/{SHEET_ID}/export?format=csv"
client = OpenAI()


# ──────────────────────────────────────────────
# Step 1 — 시트를 읽는 함수 2개
# ──────────────────────────────────────────────
def get_latest_reading():
    """최신 센서값 한 줄 (지금 상태)"""
    df = pd.read_csv(CSV_URL)                    # 호출될 때마다 새로 읽음 (최신 보장)
    last = df.iloc[-1]
    return str({
        "시각": last["시각"], "온도": last["온도"], "습도": last["습도"],
        "조도": last["조도"], "소음": last["소음"],
    })


def get_sensor_stats(sensor):
    """특정 센서의 기록 전체 통계 (최저/최고/평균)"""
    df = pd.read_csv(CSV_URL)
    col = df[sensor]
    return str({
        "센서": sensor,
        "최저": round(col.min(), 1), "최고": round(col.max(), 1),
        "평균": round(col.mean(), 1), "기록수": len(col),
    })


# ──────────────────────────────────────────────
# Step 2 — GPT에게 줄 함수 설명서
# ──────────────────────────────────────────────
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
                        "enum": ["온도", "습도", "조도", "소음"],  # 시트 1행 제목과 동일해야 함
                        "description": "통계를 낼 센서 이름",
                    }
                },
                "required": ["sensor"],
            },
        },
    },
]

available_functions = {
    "get_latest_reading": get_latest_reading,
    "get_sensor_stats": get_sensor_stats,
}


# ──────────────────────────────────────────────
# Step 3~5 — 질문 → 함수 선택 → 실행 → 최종 답변
# ──────────────────────────────────────────────
def chat(message, history):
    messages = [
        {"role": "system",
         "content": "당신은 교실 환경 모니터 도우미입니다. 제공된 함수로 센서 데이터를 조회해서 짧고 친절하게 한국어로 답하세요."},
    ]
    # 이전 대화도 GPT에게 전달 (문맥 유지)
    for user_msg, bot_msg in history:
        messages.append({"role": "user", "content": user_msg})
        messages.append({"role": "assistant", "content": bot_msg})
    messages.append({"role": "user", "content": message})

    # Step 3 — 어떤 함수를 부를지 GPT에게 물어보기
    response = client.chat.completions.create(
        model="gpt-4o-mini",
        messages=messages,
        tools=use_functions,
    )
    response_message = response.choices[0].message
    tool_calls = response_message.tool_calls

    # 함수가 필요 없는 질문이면 바로 답변
    if not tool_calls:
        return response_message.content

    # Step 4 — 함수를 (우리가) 실행하고 결과를 대화에 추가
    messages.append(response_message)
    for tool_call in tool_calls:
        fn = available_functions[tool_call.function.name]
        args = json.loads(tool_call.function.arguments)
        try:
            result = fn(**args)
        except Exception as e:
            result = f"오류: {e}"           # 시트 접근 실패 등도 GPT가 설명하게 함
        messages.append({
            "tool_call_id": tool_call.id,
            "role": "tool",
            "name": tool_call.function.name,
            "content": result,
        })

    # Step 5 — 실행 결과를 포함해 최종 답변 받기
    second = client.chat.completions.create(model="gpt-4o-mini", messages=messages)
    return second.choices[0].message.content


# ──────────────────────────────────────────────
# gradio 채팅 화면
# ──────────────────────────────────────────────
demo = gr.ChatInterface(
    fn=chat,
    title="🏫 교실 환경 챗봇",
    description="교실 온도·습도·조도·소음에 대해 물어보세요! (데이터: 우리 반 ESP32 → 구글 시트)",
    examples=["지금 교실 어때?", "오늘 최고 온도 몇 도였어?", "소음 평균 알려줘"],
)

if __name__ == "__main__":
    demo.launch()
