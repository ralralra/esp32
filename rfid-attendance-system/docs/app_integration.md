# 앱 연동 — "시뮬레이션 버튼"을 실제 ESP32 연동으로 바꾸기

지금 앱의 **[테스트] ESP32 RFID 태그 시뮬레이션** 버튼은 앱 안에서 가짜 태그를 만들어냅니다.
이걸 중계서버 호출로 바꾸면 실제 하드웨어와 연동됩니다.

앱 화면 흐름은 그대로 두고, **각 단계에서 호출할 주소만** 바꾸면 됩니다.

## 연동 코드 (JavaScript)

```js
// 중계서버 주소 (apps_script/README.md 에서 배포한 웹 앱 URL)
const RELAY_URL = "https://script.google.com/macros/s/여기에배포ID/exec";
const DEVICE_ID = "DEV001";

// 요청 도우미 — GET + URL 파라미터만 사용 (CORS 문제 없이 동작)
async function relay(params) {
  const qs = new URLSearchParams(params).toString();
  const res = await fetch(`${RELAY_URL}?${qs}`);
  const txt = await res.text();
  try { return JSON.parse(txt); } catch { return txt; }
}

// ① [출석체크 시작] 버튼 → 세션 생성
async function startAttendance() {
  return relay({
    action: "start",
    mode: "attend",
    device: DEVICE_ID,
    period: "1",            // 교시
    name: "김세이프",        // 대상 학생 (비우면 아무 학생이나 태그 가능)
    lateAfter: "09:00",     // 이 시각 이후 태그는 "지각"
    timeout: "30",          // 앱 화면의 30초 카운트다운과 맞춤
  });
}
// 참고: 등록(mode: "register") 세션은 서버가 최소 120초를 보장합니다.
// 카운트다운은 요청에 보낸 값이 아니라 응답의 timeout 값(초)으로 맞추세요.

// ② 대기 화면에서 1초마다 결과 확인 → done이 되면 결과 화면 표시
function waitForResult(onDone, onTimeout) {
  const timer = setInterval(async () => {
    const s = await relay({ action: "status", device: DEVICE_ID });
    if (s.state === "done") {
      clearInterval(timer);
      onDone(s.result);   // 예: {ok:true, name:"김세이프", status:"지각", time:"21:22:58"}
    } else if (s.state === "none") {
      clearInterval(timer);
      onTimeout();        // 30초 안에 태그가 없어 세션 만료
    }
  }, 1000);
}

// ③ 학생증 등록 화면 → 등록 세션 생성 (이후 waitForResult 동일하게 사용)
async function startRegister(student) {
  return relay({
    action: "start",
    mode: "register",
    device: DEVICE_ID,
    name: student.name,     // 예: "김세이프"
    grade: student.grade,   // "1"
    klass: student.klass,   // "3"
    number: student.number, // "7"
  });
}

// ④ 사용자가 X(닫기)를 누르면 세션 취소
async function cancelSession() {
  return relay({ action: "cancel", device: DEVICE_ID });
}
```

## 결과 화면에 표시할 값

`waitForResult` 의 `onDone(result)` 에서:

| 필드 | 출석 모드 | 등록 모드 |
|---|---|---|
| `result.ok` | 성공 여부 | 성공 여부 |
| `result.name` | 학생 이름 | 등록된 학생 이름 |
| `result.status` | `출석` / `지각` | — |
| `result.time` | 인증 시각 `21:22:58` | — |
| `result.reason` | 실패 사유: `unknown`(미등록 카드) / `mismatch`(다른 학생 카드) | `dup`(이미 등록된 카드) |

## AI Studio에 붙여넣을 요청 예시

앱을 AI Studio에서 수정한다면 아래 요청문을 그대로 쓰면 됩니다.

> 지금 "[테스트] ESP32 RFID 태그 시뮬레이션" 버튼으로 처리하는 태그 이벤트를 실제 장치 연동으로 바꿔줘.
> 중계서버 주소는 `https://script.google.com/macros/s/배포ID/exec` 이고 리더기 ID는 `DEV001`이야.
> - [출석체크 시작] 버튼: `?action=start&mode=attend&device=DEV001&period=교시&name=대상학생&lateAfter=09:00&timeout=30` 을 GET으로 호출
> - 대기 화면(30초 카운트다운) 동안: 1초마다 `?action=status&device=DEV001` 을 GET으로 폴링해서
>   `state==="done"` 이면 `result`(name, status, time)로 결과 화면을 보여주고,
>   `state==="none"` 이면 시간 초과 화면을 보여줘
> - 학생 등록 기능 추가: 학생 정보(이름·학년·반·번호) 입력 후
>   `?action=start&mode=register&device=DEV001&name=...&grade=...&klass=...&number=...` 호출하고
>   같은 방식으로 status를 폴링해서 등록 완료/실패(이미 등록된 카드)를 표시해줘
> - 닫기(X) 버튼: `?action=cancel&device=DEV001` 호출
> - 시뮬레이션 버튼은 개발용으로 남겨두되, `?action=tag&device=DEV001&uid=TEST0001` 을 호출하는 방식으로 바꿔줘
>   (그러면 시뮬레이션도 실제와 똑같은 경로로 동작해)

## 자주 걸리는 것

- **CORS**: GET + URL 파라미터만 쓰면 문제없습니다. `Content-Type: application/json` 같은
  커스텀 헤더를 붙이면 preflight 요청 때문에 실패하니 붙이지 마세요.
- **결과가 안 옴**: Apps Script 배포의 액세스 권한이 "모든 사용자"인지 확인하세요.
- **수정했는데 반영 안 됨**: Apps Script는 저장만 하면 반영되지 않습니다 →
  배포 관리에서 **새 버전으로 재배포** 해야 합니다.
- **시뮬레이션 테스트**: ESP32 없이도 브라우저 주소창에서
  `?action=tag&device=DEV001&uid=TEST0001` 을 열면 태그를 흉내낼 수 있습니다.
