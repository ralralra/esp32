# AI Studio 수정 프롬프트

만들어 둔 앱을 백엔드(Apps Script)와 실제로 연결하기 위한 지시문입니다.
**아래 회색 상자 전체를 복사해서 AI Studio 채팅에 그대로 붙여넣으세요.**

> ⚠ 붙여넣기 전에 확인: 시트의 Apps Script가 `umbrella_locker.gs`로 교체되어 있어야 합니다.
> (배포 → 배포 관리 → 연필 → **새 버전**. "새 배포"를 누르면 URL이 바뀝니다)
> 브라우저에서 `?action=plans`를 열어 JSON이 나오면 준비 완료입니다.

---

```
우리 앱을 구글 시트 백엔드에 연결해줘. 아래 규격을 정확히 지켜줘.

## 서버 주소
const SERVER_URL = "https://script.google.com/macros/s/AKfycbyQ4cU_sRLR64eNDLxUmtNUDQw8eYFfIHaq4u7ocv3uVOIrUgFsDRxrJFpANbyctOvY/exec";

## 반드시 지킬 규칙 (어기면 동작 안 함)

1. 모든 호출은 **헤더 없는 GET**으로 해줘.
   - 올바른 예: fetch(`${SERVER_URL}?action=lockers`)
   - 절대 금지: headers 옵션 추가, Content-Type 지정, JSON body로 POST
   - 이유: Apps Script는 브라우저의 preflight(OPTIONS)에 응답하지 않아서
     헤더를 붙이면 CORS 오류로 전부 실패해.

2. 서버 응답은 항상 이 모양이야:
   - 성공: { "ok": true, ...데이터 }
   - 실패: { "ok": false, "error": "사람이 읽을 수 있는 한국어 메시지" }
   실패면 error 값을 그대로 사용자에게 보여줘. 자체 메시지로 바꾸지 마.
   (서버가 "이미 대여 중이에요" 같은 정확한 이유를 한국어로 보내줌)

3. 회원번호(user_id)는 localStorage에 저장해서 재사용해줘.
   키 이름: umbrella_user_id, umbrella_user_name
   한 번 등록한 사용자는 앱을 다시 열어도 개인정보를 다시 입력하지 않아야 해.

4. 요금제·보관함·우산 목록을 **코드에 하드코딩하지 마.**
   전부 서버에서 받아와서 화면에 그려줘. (시트를 고치면 앱에 바로 반영돼야 함)

5. 서버 호출은 모두 try/catch로 감싸고, 로딩 중에는 버튼을 비활성화해줘.
   같은 버튼을 두 번 눌러 대여가 두 번 되는 일이 없어야 해.

## 화면별 연결

### ① 홈
- localStorage에 umbrella_user_id가 있으면 [우산 대여하기] → 보관함 찾기(③)로,
  없으면 개인정보 입력(②)으로 보내줘.
- [우산 반납하기] → 반납 화면(⑨)

### ② 개인정보 입력
호출: ?action=register&name={이름}&phone={전화번호}
응답: { ok, user_id, name, existing }
- 성공하면 user_id와 name을 localStorage에 저장하고 보관함 찾기(③)로 이동
- existing이 true면 이미 가입된 번호라는 뜻 (에러 아님, 그냥 진행)
- 개인정보 동의 체크박스를 안 눌렀으면 서버 호출 없이 안내만

### ③ 보관함 찾기
호출: ?action=lockers
응답: { ok, lockers: [{ locker_id, locker_name, location, total_slots, available_slots, status }] }
- 목록에 locker_name과 "사용 가능 {available_slots}개"를 표시해줘
- available_slots가 0이거나 status가 "active"가 아니면 선택 불가로 흐리게
- 지도는 실제 지도 API 대신 **정적 이미지 한 장**을 쓰고 그 위에 핀을 고정 배치해줘
  (거리 표시 50m/120m 같은 값은 화면에 고정 텍스트로 둬도 돼)
- 선택한 locker_id를 다음 화면으로 넘겨줘

### ④ QR 코드 스캔
- html5-qrcode로 후면 카메라 스캔. QR에는 "L001" 같은 locker_id가 들어 있어.
- 스캔한 값이 ?action=lockers 목록에 있는 locker_id인지 확인하고,
  없으면 "등록되지 않은 보관함이에요" 안내
- **[직접 번호 입력하기] 버튼을 반드시 남겨줘.** 카메라 권한이 막히면
  QR 없이도 진행할 수 있어야 해 (입력값은 대문자로 변환해서 처리)

### ⑤ 우산 선택
호출: ?action=status&locker_id={선택한 보관함}
응답: { ok, umbrellas: [{ umbrella_id, locker_id, slot_no, status }] }
- slot_no를 2자리로("01","02") 표시하고, status가 "available"이면 "사용 가능",
  아니면 "대여 중"으로 표시하고 선택 불가 처리
- 선택한 umbrella_id를 다음 화면으로 넘겨줘

### ⑥ 요금제 선택
호출: ?action=plans
응답: { ok, plans: [{ plan_id, plan_name, hours, price, extra_24h_price }] }
- plan_name과 price(천단위 콤마)를 표시하고,
  "초과 24시간당 +{extra_24h_price}원"을 작은 글씨로 같이 보여줘
- 24/48/72시간을 코드에 직접 쓰지 말고 이 응답으로만 그려줘
- 선택한 plan_id를 넘기고, 계좌 등록을 아직 안 했으면 ⑦로, 했으면 ⑧로

### ⑦ 결제 수단 등록 (최초 1회)
호출: ?action=register_account&user_id={저장된 ID}&account={계좌번호}
응답: { ok, account_masked }
- 서버가 계좌를 마스킹해서 저장해 (123-456-789012 → 123-***-9012).
  앱에서는 원본 계좌번호를 저장하지 마.
- 성공하면 localStorage에 umbrella_has_account = "1" 저장하고 ⑧로

### ⑧ 대여 완료
호출: ?action=rent&user_id={ID}&locker_id={④에서 정한 값}&umbrella_id={⑤}&plan_id={⑥}
응답: { ok, rental_id, umbrella_id, locker_id, slot_no, plan_hours, plan_price, expected_return }
- 화면에 보관함 이름, 우산 번호(slot_no 2자리), 이용 요금제
  ({plan_hours}시간 / {plan_price}원), 대여 시작 시각, 반납 예정(expected_return)을 표시
- expected_return은 ISO 시각 문자열이니 읽기 좋은 형식으로 변환해줘
- 실패 시 error 메시지를 그대로 보여줘 ("이미 대여 중이에요 — 먼저 반납하세요" 등)

### ⑨ 반납 + ⑩ 결제 완료
호출: ?action=return&user_id={ID}
응답: { ok, rental_id, slot_no, duration_min, plan_price, extra_charge, amount,
        payment_id, account_masked }
- **반납과 결제가 이 호출 하나로 끝나.** 별도 결제 호출을 만들지 마.
- ⑩ 결제 완료 화면에 그대로 표시:
  결제 금액 = amount (천단위 콤마)
  이용 시간 = duration_min을 "23시간 40분" 형태로 변환
  기본 요금 = plan_price, 초과 요금 = extra_charge (0이면 "초과 요금 없음")
  결제 계좌 = account_masked
- 실패 시 error 그대로 표시 ("대여 중인 우산이 없어요" 등)

### 내역 화면
호출: ?action=history&user_id={ID}
응답: { ok, rentals: [...], payments: [...] }  (최신이 배열 앞쪽)
- rentals: umbrella_id, rental_time, return_time, status, payment_status
  return_time이 비어 있으면 "대여 중"으로 표시
- payments: amount, account_masked, payment_time, status

## 마지막 점검
- 앱 어디에도 우산·보관함·요금제 목록이 하드코딩되어 있으면 전부 제거하고
  위 API 호출로 바꿔줘.
- fetch에 headers나 Content-Type이 남아 있으면 전부 제거해줘.
```

---

## 붙여넣은 뒤 확인할 것

| 확인 | 정상이면 |
|---|---|
| ② 이름·전화번호 입력 | 시트 `users` 탭에 새 줄이 생기고 U002 같은 ID가 발급됨 |
| ③ 보관함 목록 | 도서관/학생회관/정문 3곳이 뜨고 잔여 개수가 표시됨 |
| ⑥ 요금제 | 24/48/72시간이 시트 값 그대로 뜸 (시트에서 가격 바꾸면 앱도 바뀜) |
| ⑧ 대여 | 시트 `rentals`에 active 한 줄 + `umbrellas`가 rented로 바뀜 |
| ⑨⑩ 반납 | `payments`에 결제 한 줄 + 이용 시간·금액이 화면에 표시됨 |

문제가 생기면 **브라우저 개발자도구 → Console**을 먼저 보세요:

| 콘솔 메시지 | 원인 |
|---|---|
| `Failed to fetch` / CORS | fetch에 headers가 남아 있음 → 전부 제거 |
| `올바른 action이 없습니다` | 시트 Apps Script가 예전 버전 → 교체 후 "새 버전" 배포 |
| `등록되지 않은 사용자` | localStorage가 비워짐 → ②에서 다시 등록 |
