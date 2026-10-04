# 앱 화면별 연동 코드

기획안 10개 화면을 순서대로, **각 화면에서 무엇을 호출하고 무엇이 돌아오는지** 정리했습니다.
먼저 [`api.js`](api.js)를 앱에 넣고 맨 위 `SERVER_URL`만 시트1 탭의 `/exec` 주소로 바꾸세요.

> AI Studio에 지시할 때는 이 문서의 코드 블록을 그대로 붙여넣고
> "이 함수를 화면 ○○에 연결해줘"라고 하면 됩니다.

---

## ① 시작 / 홈

대여 버튼을 누를 때 **회원 등록 여부로 분기**합니다. 처음이면 ②로, 이미 등록했으면 ③으로.

```javascript
function onClickRent() {
  if (UmbrellaAPI.isRegistered()) {
    goTo("보관함찾기");        // ③ — 두 번째부터는 개인정보 입력 생략
  } else {
    goTo("개인정보입력");      // ②
  }
}

function onClickReturn() {
  if (!UmbrellaAPI.isRegistered()) return alert("먼저 대여를 진행해주세요");
  goTo("반납");               // ⑨
}
```

---

## ② 개인정보 입력 (최초 1회)

이름·전화번호를 보내면 `user_id`가 만들어지고 **폰에 저장**됩니다.
같은 전화번호가 이미 있으면 새로 만들지 않고 기존 ID를 돌려줘요(`existing: true`).

```javascript
async function onSubmitInfo(name, phone, agreed) {
  if (!agreed)        return alert("개인정보 이용 동의가 필요해요");
  if (!name || !phone) return alert("이름과 전화번호를 모두 입력해주세요");

  try {
    const r = await UmbrellaAPI.register(name, phone);
    console.log("내 회원번호:", r.user_id);   // 예: U002
    goTo("보관함찾기");                        // ③
  } catch (e) {
    alert("등록 실패: " + e.message);
  }
}
```

---

## ③ 보관함 찾기 / 선택

지도는 **이미지로 대체**하고, 그 위에 핀을 얹거나 아래 목록만 보여줘도 충분합니다.
`available_slots`가 실시간 대여 가능 개수입니다.

```javascript
async function loadLockers() {
  const lockers = await UmbrellaAPI.getLockers();

  lockers.forEach(l => {
    addLockerCard({
      id:    l.locker_id,                        // "L001"
      name:  l.locker_name,                      // "도서관 앞 보관함"
      desc:  `사용 가능 ${l.available_slots}개 / 전체 ${l.total_slots}개`,
      disabled: l.status !== "active" || l.available_slots === 0
    });
  });
}

function onSelectLocker(lockerId) {
  saveState("locker_id", lockerId);   // ④⑤에서 계속 씀
  goTo("QR스캔");                     // ④
}
```

---

## ④ QR 코드 스캔

보관함에 **`L001` 같은 locker_id를 담은 QR을 인쇄해서 붙여두면** 스캔으로 인식됩니다.
카메라가 안 될 때를 대비해 "직접 번호 입력" 경로도 꼭 남겨두세요 (박람회 현장 대비).

```javascript
// html5-qrcode 사용 — AI Studio에 "카메라로 QR을 스캔하는 기능 추가해줘"로 생성
const scanner = new Html5Qrcode("reader");

scanner.start({ facingMode: "environment" }, { fps: 10, qrbox: 250 },
  async (decodedText) => {                 // decodedText = "L001"
    await scanner.stop();
    onLockerScanned(decodedText.trim());
  });

async function onLockerScanned(lockerId) {
  const lockers = await UmbrellaAPI.getLockers();
  if (!lockers.some(l => l.locker_id === lockerId))
    return alert("등록되지 않은 보관함이에요: " + lockerId);

  saveState("locker_id", lockerId);
  goTo("우산선택");                        // ⑤
}

// 카메라 실패 대비 — "직접 번호 입력하기" 버튼
function onManualInput(text) { onLockerScanned(text.trim().toUpperCase()); }
```

---

## ⑤ 우산 선택

그 보관함의 우산만 불러와 `status`로 사용 가능/대여 중을 구분합니다.

```javascript
async function loadUmbrellas() {
  const lockerId  = loadState("locker_id");
  const umbrellas = await UmbrellaAPI.getUmbrellas(lockerId);

  umbrellas.forEach(u => {
    addUmbrellaTile({
      id:       u.umbrella_id,                       // "UB001"
      label:    String(u.slot_no).padStart(2, "0"),  // "01"
      status:   u.status === "available" ? "사용 가능" : "대여 중",
      disabled: u.status !== "available"
    });
  });
}

function onSelectUmbrella(umbrellaId) {
  saveState("umbrella_id", umbrellaId);
  goTo("요금제선택");                       // ⑥
}
```

---

## ⑥ 요금제 선택 (시간제)

요금제는 시트 `plans` 탭에서 읽어옵니다. 가격을 바꾸고 싶으면 **시트만 고치면 앱에 바로 반영**돼요.

```javascript
async function loadPlans() {
  const plans = await UmbrellaAPI.getPlans();

  plans.forEach(p => {
    addPlanRow({
      id:    p.plan_id,                              // "P001"
      name:  p.plan_name,                            // "24시간"
      price: p.price.toLocaleString() + "원",        // "2,000원"
      note:  `초과 24시간당 +${p.extra_24h_price.toLocaleString()}원`
    });
  });
}

function onSelectPlan(planId) {
  saveState("plan_id", planId);
  // 계좌를 아직 등록 안 했으면 ⑦로, 이미 했으면 바로 대여(⑧)
  goTo(hasAccount() ? "대여처리" : "결제수단등록");
}
```

---

## ⑦ 결제 수단 등록 (계좌 · 최초 1회)

후불 결제용 계좌를 받습니다. 서버는 **마스킹한 형태만 저장**하고 원본은 저장하지 않아요.

```javascript
async function onSubmitAccount(bank, accountNo, holder, agreed) {
  if (!agreed) return alert("자동 요금 이용 동의가 필요해요");

  try {
    const r = await UmbrellaAPI.registerAccount(accountNo);
    console.log("등록된 계좌:", r.account_masked);   // "123-***-9012"
    localStorage.setItem("umbrella_has_account", "1");
    goTo("대여처리");                                 // ⑧
  } catch (e) {
    alert("계좌 등록 실패: " + e.message);
  }
}

function hasAccount() { return localStorage.getItem("umbrella_has_account") === "1"; }
```

---

## ⑧ 대여 완료

④⑤⑥에서 모은 값을 한 번에 보냅니다. **이 호출 하나로** 대여 기록 생성 + 우산 상태 변경 +
보관함 잔여 수량 갱신 + (보드가 있으면) 슬롯 열림까지 전부 처리됩니다.

```javascript
async function doRent() {
  try {
    const r = await UmbrellaAPI.rent(
      loadState("locker_id"),
      loadState("umbrella_id"),
      loadState("plan_id")
    );

    showRentResult({
      locker:   r.locker_id,
      umbrella: String(r.slot_no).padStart(2, "0") + "번",
      fee:      `${r.plan_hours}시간 (${r.plan_price.toLocaleString()}원)`,
      start:    formatTime(new Date()),
      due:      formatTime(new Date(r.expected_return))
    });
  } catch (e) {
    alert(e.message);   // "이미 대여 중이에요" 등 서버 메시지가 그대로 옴
  }
}
```

> 💡 **대여 실패 처리**: 보드를 연결한 경우, 슬롯이 열렸는데 우산을 안 꺼내면
> 보드가 자동으로 대여를 취소합니다(`action=cancel`). 앱은 신경 쓰지 않아도 돼요.

---

## ⑨ 반납 + ⑩ 결제 완료

**반납과 정산이 한 번의 호출로 끝납니다.** 실제 이용 시간을 재서 요금을 계산하고
결제 기록까지 남긴 뒤, 그 결과를 돌려줍니다 → 바로 ⑩ 화면에 뿌리면 됩니다.

```javascript
async function doReturn() {
  try {
    const r = await UmbrellaAPI.returnUmbrella();

    // ⑩ 결제 완료 화면
    showPaymentResult({
      amount:   r.amount.toLocaleString() + "원",       // 총 결제 금액
      duration: formatDuration(r.duration_min),          // "23시간 40분"
      basic:    r.plan_price.toLocaleString() + "원",
      extra:    r.extra_charge > 0
                  ? `초과 요금 +${r.extra_charge.toLocaleString()}원`
                  : "초과 요금 없음",
      account:  r.account_masked,                        // "123-***-9012"
      time:     formatTime(new Date())
    });
  } catch (e) {
    alert(e.message);   // "대여 중인 우산이 없어요" 등
  }
}

function formatDuration(min) {
  const h = Math.floor(min / 60), m = min % 60;
  return h > 0 ? `${h}시간 ${m}분` : `${m}분`;
}
```

---

## 내역 화면

이용 기록과 결제 기록을 함께 받습니다 (최신이 위).

```javascript
async function loadHistory() {
  const { rentals, payments } = await UmbrellaAPI.getHistory();

  rentals.forEach(r => addHistoryRow({
    umbrella: r.umbrella_id,
    period:   `${formatTime(new Date(r.rental_time))} ~ ` +
              (r.return_time ? formatTime(new Date(r.return_time)) : "대여 중"),
    state:    r.status === "active" ? "대여 중" : "반납 완료"
  }));

  payments.forEach(p => addPaymentRow({
    amount:  p.amount.toLocaleString() + "원",
    account: p.account_masked,
    time:    formatTime(new Date(p.payment_time)),
    state:   p.status                                   // "paid"
  }));
}
```

---

## 화면 ↔ API 한눈표

| 화면 | 호출 | 돌아오는 값 |
|---|---|---|
| ① 홈 | `isRegistered()` (서버 호출 없음) | true/false |
| ② 개인정보 | `register(name, phone)` | `user_id`, `existing` |
| ③ 보관함 | `getLockers()` | 보관함 목록 + 잔여 수량 |
| ④ QR | `getLockers()` 로 유효성 확인 | — |
| ⑤ 우산 선택 | `getUmbrellas(lockerId)` | 우산별 `slot_no`, `status` |
| ⑥ 요금제 | `getPlans()` | 요금제 목록 |
| ⑦ 계좌 등록 | `registerAccount(account)` | `account_masked` |
| ⑧ 대여 완료 | `rent(locker, umbrella, plan)` | `rental_id`, `expected_return` |
| ⑨⑩ 반납·결제 | `returnUmbrella()` | `amount`, `duration_min`, `payment_id` |
| 내역 | `getHistory()` | `rentals`, `payments` |

## 자주 겪는 문제

| 증상 | 원인 / 해결 |
|---|---|
| `Failed to fetch` / CORS 오류 | fetch에 헤더를 붙였거나 JSON POST를 씀 → **GET만 사용** (api.js 그대로 쓰면 안전) |
| `올바른 action이 없습니다` | 시트에 배포된 코드가 예전 버전 → `umbrella_locker.gs`로 교체 후 **배포 관리 → 연필 → 새 버전** |
| `등록되지 않은 사용자` | 폰 저장소가 비워짐 → ②에서 다시 등록 |
| `이미 대여 중이에요` | 한 사람당 한 개 규칙 → 먼저 반납 |
| 대여는 됐는데 문이 안 열림 | 보드 미연결/전원 문제. 앱·시트는 정상 동작하므로 데모는 계속 가능 |
