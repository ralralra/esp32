# AI Studio 전체 빌드 프롬프트 (처음부터 만들기)

앱을 **백지에서** 만들 때 쓰는 지시문입니다.
아래 회색 상자 전체를 복사해서 AI Studio에 붙여넣으세요.

> ⚠ 먼저 확인: 브라우저에서 `/exec?action=plans`를 열어 요금제 JSON이 나와야 합니다.
> 안 나오면 시트 Apps Script를 `umbrella_locker.gs`로 교체하고
> **배포 → 배포 관리 → 연필 → 새 버전**으로 갱신하세요.

이미 만든 앱을 고치는 용도라면 → [`ai_studio_prompt.md`](ai_studio_prompt.md)

---

```
'SMART UMBRELLA'라는 모바일 웹앱을 만들어줘.
학교 우산 대여 서비스이고, 구글 시트를 DB로 쓰는 백엔드가 이미 완성돼 있어.
아래 규격에 정확히 맞춰서 만들어줘.

═══════════════════════════════════════════
1. 서버 연결 — 이 규칙을 어기면 앱이 전혀 동작하지 않아
═══════════════════════════════════════════

const SERVER_URL = "https://script.google.com/macros/s/AKfycbyQ4cU_sRLR64eNDLxUmtNUDQw8eYFfIHaq4u7ocv3uVOIrUgFsDRxrJFpANbyctOvY/exec";

[규칙 1] 모든 서버 호출은 헤더 없는 GET이어야 해.
  올바른 예: const res = await fetch(`${SERVER_URL}?action=lockers`);
  절대 금지: headers 옵션, Content-Type 지정, JSON body로 POST, axios 커스텀 설정
  이유: Google Apps Script는 브라우저의 preflight(OPTIONS) 요청에 응답하지 않아.
        헤더를 하나라도 붙이면 모든 요청이 CORS 오류로 실패해.

[규칙 2] 서버 응답 형식은 항상 아래 둘 중 하나야.
  성공: { "ok": true, ...데이터 }
  실패: { "ok": false, "error": "한국어 오류 메시지" }
  → 실패면 error 값을 그대로 사용자에게 보여줘. 임의로 문구를 바꾸지 마.
     서버가 "이미 대여 중이에요 — 먼저 반납하세요" 같은 정확한 이유를 한국어로 보내줌.

[규칙 3] 공통 호출 함수를 하나 만들어서 모든 화면이 그것만 쓰게 해줘.

  async function callApi(action, params = {}) {
    const q = new URLSearchParams({ action, ...params });
    const res = await fetch(`${SERVER_URL}?${q}`);
    if (!res.ok) throw new Error(`서버 응답 오류 (${res.status})`);
    const data = await res.json();
    if (data.ok === false) throw new Error(data.error || "알 수 없는 오류");
    return data;
  }

[규칙 4] 모든 호출은 try/catch로 감싸고, 통신 중에는 버튼을 비활성화하고
  로딩 표시를 보여줘. 같은 버튼을 두 번 눌러 대여가 두 번 되면 안 돼.

[규칙 5] 보관함·우산·요금제 목록을 코드에 하드코딩하지 마.
  전부 서버에서 받아와서 그려줘. 시트를 고치면 앱에 바로 반영돼야 해.

═══════════════════════════════════════════
2. 저장하는 값 (localStorage)
═══════════════════════════════════════════

umbrella_user_id      : 회원번호 (예: "U002") — 등록 후 계속 재사용
umbrella_user_name    : 이름
umbrella_has_account  : 계좌 등록 완료 시 "1"

한 번 등록한 사용자는 앱을 다시 열어도 개인정보를 다시 입력하지 않아야 해.

═══════════════════════════════════════════
3. 디자인
═══════════════════════════════════════════

- 모바일 세로 화면 전용 (max-width 420px, 가운데 정렬)
- 메인 컬러: 인디고/보라 (#5B5BD6), 배경 흰색, 카드는 둥근 모서리(16px)와 옅은 그림자
- 성공 표시는 초록(#16A34A), 경고/대여중은 주황(#D97706)
- 한글 폰트, 큼직한 터치 영역, 버튼은 꽉 찬 너비
- 하단 탭바 4개: 홈 / 내역 / 보관함 / 마이
- 화면 전환은 페이지 이동이 아니라 한 페이지 안에서 상태로 전환(SPA)

═══════════════════════════════════════════
4. 화면 구성 (총 10단계 + 내역)
═══════════════════════════════════════════

──────────── ① 홈 ────────────
- 상단 'SMART UMBRELLA' 로고, 우산 일러스트
- 문구: "비 오는 날, 스마트하게 우산을 빌려보세요"
- 버튼 2개: [우산 대여하기](진한 버튼) [우산 반납하기](연한 버튼)
- 동작:
  대여하기 → localStorage에 umbrella_user_id가 있으면 ③으로, 없으면 ②로
  반납하기 → umbrella_user_id가 없으면 "먼저 대여를 진행해주세요" 안내, 있으면 ⑨로

──────────── ② 개인정보 입력 (최초 1회) ────────────
- 입력: 이름, 전화번호 / 체크박스: "개인정보 이용 동의 (필수)"
- [다음] 버튼

호출: callApi("register", { name, phone })
응답: { ok, user_id, name, existing }

- 동의 안 했거나 빈칸이면 서버 호출 없이 안내만
- 성공 시 user_id와 name을 localStorage에 저장하고 ③으로 이동
- existing이 true면 이미 가입된 번호라는 뜻 → 오류가 아니니 그대로 진행

──────────── ③ 보관함 찾기 ────────────
호출: callApi("lockers")
응답: { ok, lockers: [{ locker_id, locker_name, location, total_slots, available_slots, status }] }

- 상단에 지도 영역 — 실제 지도 API를 쓰지 말고 회색 배경의 정적 지도 이미지 느낌으로
  만들고 그 위에 보관함 핀을 고정 위치에 표시해줘 (데모용)
- 아래에 보관함 목록 카드:
    제목 = locker_name (예: "도서관 앞 보관함")
    설명 = "사용 가능 {available_slots}개 / 전체 {total_slots}개"
    거리 = "50m" 같은 고정 텍스트 (실제 GPS 계산 안 함)
- available_slots가 0이거나 status가 "active"가 아니면 흐리게 처리하고 선택 불가
- 카드를 누르면 그 locker_id를 상태에 저장하고 ④로 이동

──────────── ④ QR 코드 스캔 ────────────
- 화면 중앙에 사각 스캔 프레임, 안내문 "보관함의 QR 코드를 스캔해주세요"
- html5-qrcode 라이브러리를 CDN으로 불러와서 후면 카메라로 스캔
  (QR 안에는 "L001" 같은 보관함 번호가 들어 있어)
- 스캔한 값이 ③에서 받은 locker_id 목록에 있는지 확인하고,
  없으면 "등록되지 않은 보관함이에요: {값}" 안내
- 유효하면 그 locker_id를 저장하고 ⑤로 이동

★ [직접 번호 입력하기] 버튼을 반드시 넣어줘.
  카메라 권한이 막히거나 실패해도 번호 입력으로 진행할 수 있어야 해.
  입력값은 앞뒤 공백 제거 + 대문자로 변환해서 같은 검증을 거치게 해줘.

──────────── ⑤ 우산 선택 ────────────
호출: callApi("status", { locker_id: 선택한보관함 })
응답: { ok, umbrellas: [{ umbrella_id, locker_id, slot_no, status }] }

- 우산을 타일 그리드로 표시:
    큰 글씨 = slot_no를 2자리로 ("01", "02")
    작은 글씨 = status가 "available"이면 "사용 가능", 아니면 "대여 중"
- 사용 불가 타일은 회색으로 흐리게 + 선택 불가
- 선택하면 umbrella_id를 저장하고 [선택 확인] 버튼으로 ⑥으로 이동

──────────── ⑥ 요금제 선택 (시간제) ────────────
호출: callApi("plans")
응답: { ok, plans: [{ plan_id, plan_name, hours, price, extra_24h_price }] }

- 안내문 "이용 시간을 선택해주세요"
- 각 요금제를 행으로: 왼쪽 plan_name("24시간"), 오른쪽 price를 천단위 콤마로("2,000원")
- 목록 아래에 작은 글씨로 "추가 24시간당 +{extra_24h_price}원"
- 24/48/72를 코드에 직접 쓰지 말고 반드시 이 응답으로만 그려줘
- 선택 후 [다음]:
    umbrella_has_account가 "1"이면 → ⑧ 대여 실행
    아니면 → ⑦ 계좌 등록

──────────── ⑦ 결제 수단 등록 (최초 1회) ────────────
- 입력: 은행 선택(드롭다운), 계좌번호, 예금주
- 체크박스: "자동 요금 결제 동의 (필수)"
- 안내문 "후불 결제를 위한 계좌를 등록하세요."

호출: callApi("register_account", { user_id: 저장된ID, account: 계좌번호 })
응답: { ok, user_id, account_masked }

- 서버가 계좌를 마스킹해서 저장해 (123-456-789012 → "123-***-9012")
  앱에는 원본 계좌번호를 저장하지 마.
- 성공하면 localStorage의 umbrella_has_account를 "1"로 저장하고 ⑧ 실행

──────────── ⑧ 대여 완료 ────────────
호출: callApi("rent", {
        user_id: 저장된ID, locker_id: ④값, umbrella_id: ⑤값, plan_id: ⑥값 })
응답: { ok, rental_id, umbrella_id, locker_id, slot_no,
        plan_hours, plan_price, expected_return }

- 화면 상단에 파란 체크 아이콘 + "대여가 완료되었습니다!"
- 정보 목록으로 표시:
    보관함     = 보관함 이름 (③에서 받은 locker_name)
    우산 번호  = slot_no를 2자리로 + "번"
    이용 요금  = "{plan_hours}시간 ({plan_price}원)"  ※ 가격은 천단위 콤마
    대여 시작  = 현재 시각
    반납 예정  = expected_return (ISO 시각 문자열 → "2026-05-11 14:30" 형태로 변환)
- [확인] 버튼 → 홈으로
- 실패하면 error 메시지를 그대로 보여줘
  (예: "이미 대여 중이에요 — 먼저 반납하세요", "이 보관함에는 빌릴 수 있는 우산이 없어요")

──────────── ⑨ 우산 반납 → ⑩ 결제 완료 ────────────
★ 중요: 반납과 결제가 서버 호출 한 번으로 함께 처리돼.
  별도의 결제 API를 만들지 마.

⑨ 화면: 보관함 일러스트 + "우산을 보관함에 넣어주세요"
  안내문 "반납 후 화면의 버튼을 눌러주세요. 반납이 완료되면 요금이 정산됩니다."
  [반납 완료] 버튼

호출: callApi("return", { user_id: 저장된ID })
응답: { ok, rental_id, slot_no, duration_min, plan_price,
        extra_charge, amount, payment_id, account_masked }

⑩ 결제 완료 화면 (위 응답을 그대로 표시):
    초록 체크 아이콘 + "결제가 완료되었습니다"
    결제 금액 = amount (천단위 콤마 + "원")
    이용 시간 = duration_min(분)을 "23시간 40분" 형태로 변환
                (60으로 나눠 시간, 나머지 분. 1시간 미만이면 "40분")
    기본 요금 = plan_price
    초과 요금 = extra_charge가 0보다 크면 "+{extra_charge}원",
                0이면 "초과 요금 없음"
    결제 계좌 = account_masked (예: "123-***-9012")
    결제 시간 = 현재 시각
    [확인] 버튼 → 홈으로

- 실패 시 error 그대로 표시 (예: "대여 중인 우산이 없어요")

──────────── 내역 화면 (하단 탭) ────────────
호출: callApi("history", { user_id: 저장된ID })
응답: { ok, rentals: [...], payments: [...] }   ※ 배열 앞쪽이 최신

rentals 각 항목: { rental_id, umbrella_id, locker_id, rental_time,
                   return_time, duration_min, status, payment_status }
  - return_time이 비어 있으면 "대여 중"으로 표시, 있으면 "반납 완료"
  - 기간 = "{rental_time} ~ {return_time}" (읽기 좋은 형식으로)

payments 각 항목: { payment_id, rental_id, amount, method,
                    account_masked, payment_time, status }
  - 금액(천단위 콤마), 결제 계좌, 결제 시간을 카드로 표시

- 내역이 비어 있으면 "아직 이용 내역이 없어요" 안내

═══════════════════════════════════════════
5. 마지막 점검
═══════════════════════════════════════════

- fetch 어디에도 headers나 Content-Type이 없어야 함
- 보관함/우산/요금제가 하드코딩된 곳이 없어야 함
- 모든 서버 호출에 try/catch와 로딩 상태가 있어야 함
- 서버 오류 메시지(error)를 그대로 사용자에게 보여줘야 함
- 새로고침해도 로그인(user_id)이 유지돼야 함
```

---

## 만들고 나서 순서대로 확인

| 순서 | 확인 | 정상이면 |
|---|---|---|
| 1 | ② 이름·전화 입력 | 시트 `users` 탭에 새 줄 + `U002` 같은 ID 발급 |
| 2 | ③ 보관함 목록 | 3곳이 뜨고 잔여 개수 표시 |
| 3 | ⑥ 요금제 | 시트 `plans` 값 그대로 (시트에서 가격 바꾸면 앱도 바뀜) |
| 4 | ⑧ 대여 | `rentals`에 active 한 줄 + `umbrellas`가 rented로 |
| 5 | ⑨⑩ 반납 | `payments`에 결제 한 줄 + 이용 시간·금액 표시 |

시연 전 초기화: Apps Script 편집기에서 `resetDemo()` 실행
→ 대여·결제 기록을 비우고 우산을 전부 `available`로 되돌립니다.

## 오류가 나면

| 콘솔 메시지 | 원인 |
|---|---|
| `Failed to fetch` / CORS | fetch에 headers가 붙음 → 전부 제거 |
| `올바른 action이 없습니다` | 시트 Apps Script가 예전 버전 → 교체 후 "새 버전" 배포 |
| `등록되지 않은 사용자` | localStorage 초기화됨 → ②에서 다시 등록 |
| `이미 대여 중이에요` | 한 사람당 한 개 규칙 → 먼저 반납 |
