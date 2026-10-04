/*
  SMART UMBRELLA — Apps Script 백엔드 (구글 시트 = DB, 후불제 시간제 대여)
  ----------------------------------------------------------------------
  동아리 박람회 데모 기준: 결제는 "가상 결제"(실제 출금 없이 payments 기록 생성),
  지도는 앱의 정적 이미지, QR은 보관함에 인쇄해 붙인 locker_id QR을 사용합니다.

  시트 탭: users / lockers / umbrellas / rentals / payments / plans
           (commands 탭은 없으면 자동 생성 — ESP32 명령 큐)
  ※ 처음 쓰는 빈 시트라면 편집기에서 setupSheets()를 한 번 실행하세요.

  배포: 확장 프로그램 → Apps Script → 배포 → 새 배포 → 웹 앱
        "액세스 권한: 모든 사용자" → /exec URL을 앱과 ESP32에 넣기
  ※ 이미 배포한 적이 있으면 "배포 관리 → 연필 → 새 버전"으로 갱신하면 URL이 유지됩니다.

  브라우저 테스트 (전부 GET):
    URL?action=register&name=홍길동&phone=010-1234-5678   → 회원 등록(중복 전화면 기존 ID)
    URL?action=register_account&user_id=U001&account=123-456-789012
    URL?action=lockers                                    → 보관함 목록
    URL?action=status&locker_id=L001                      → 그 보관함 우산 현황
    URL?action=rent&user_id=U001&locker_id=L001&umbrella_id=UB002&plan_id=P001
    URL?action=return&user_id=U001                        → 반납 + 후불 정산(가상 결제)
    URL?action=history&user_id=U001                       → 이용·결제 내역
    URL?action=cmd&locker_id=L001                         → ESP32용 (읽으면 비워짐)
      ⚠ 브라우저로 열면 보드가 받아야 할 명령을 가로챕니다 — 문 안 열릴 때만 확인용으로!

  ★ 대여/반납/정산은 rentals·umbrellas·lockers·payments를 이 스크립트가
    "한 번에" 갱신합니다. 시트를 손으로 고치면 정합성이 깨져요!
*/

const TAB = {                    // ★ 실제 시트 탭 이름과 다르면 여기만 고치세요
  users:     "users",
  lockers:   "lockers",
  umbrellas: "umbrellas",
  rentals:   "rentals",
  payments:  "payments",
  plans:     "plans",
  commands:  "commands"
};
const DEFAULT_PLAN = "P001";     // 요금제 미지정 시 24시간권
const DEFAULT_EXTRA_24H = 1000;  // plans에서 못 찾을 때 초과 24시간당 요금

// ───────────────────────── 진입점 ─────────────────────────
function doGet(e)  { return route(e); }
function doPost(e) { return route(e); }   // 앱이 POST로 보내도 동일하게 동작

function route(e) {
  const a = (e.parameter.action || "").toLowerCase();
  const lock = LockService.getScriptLock();   // 동시 요청 충돌 방지
  lock.waitLock(5000);
  try {
    if (a === "register")         return json(register(e));
    if (a === "register_account") return json(registerAccount(e));
    if (a === "lockers")          return json(lockerList());
    if (a === "plans")            return json(planList());
    if (a === "status")           return json(status(e));
    if (a === "rent")             return json(rent(e));
    if (a === "return")           return json(returnUmbrella(e));
    if (a === "history")          return json(history(e));
    if (a === "cmd")              return text(popCommand(e.parameter.locker_id));
    if (a === "report")           return json(report(e));
    if (a === "cancel")           return json(cancelRental(e));
    if (a === "return_failed")    return json(returnFailed(e));
    if (a === "getumbrella")      return json(getUmbrella(e));   // 기존 앱 호환
    return json({ ok: false, error:
      "action을 지정하세요 (register/register_account/lockers/plans/status/rent/return/" +
      "history/cmd/report/cancel/return_failed)" });
  } catch (err) {
    return json({ ok: false, error: String(err) });
  } finally {
    lock.releaseLock();
  }
}

// ─────────────────── 회원 (앱 화면 ②·⑦) ───────────────────

// 회원 등록: 같은 전화번호가 있으면 기존 user_id 반환 (최초 1회 입력 UX)
function register(e) {
  const name  = e.parameter.name;
  const phone = e.parameter.phone;
  if (!name || !phone) return { ok: false, error: "name과 phone이 필요해요" };

  const existing = findRow(TAB.users, "phone", phone);
  if (existing) return { ok: true, user_id: existing.user_id,
                         name: existing.name, existing: true };

  const userId = nextId(TAB.users, "user_id", "U");
  appendByHeader(TAB.users, {
    user_id: userId, name: name, phone: phone,
    email: e.parameter.email || "",
    created_at: new Date(), status: "active"
  });
  return { ok: true, user_id: userId, name: name, existing: false };
}

// 결제 수단(계좌) 등록 — 가상 결제용: 마스킹해서 users 탭에 저장 (최초 1회)
function registerAccount(e) {
  const userId  = e.parameter.user_id;
  const account = e.parameter.account;
  if (!userId || !account) return { ok: false, error: "user_id와 account가 필요해요" };
  if (!findRow(TAB.users, "user_id", userId))
    return { ok: false, error: "등록되지 않은 사용자: " + userId };

  ensureColumn(TAB.users, "account_masked");
  const masked = maskAccount(account);
  updateRow(TAB.users, "user_id", userId, { account_masked: masked });
  return { ok: true, user_id: userId, account_masked: masked };
}

function maskAccount(acc) {       // 123-456-789012 → 123-***-9012 (원본은 저장 안 함!)
  const digits = String(acc).replace(/[^0-9]/g, "");
  if (digits.length < 7) return "***";
  return digits.slice(0, 3) + "-***-" + digits.slice(-4);
}

// ─────────────────── 조회 (앱 화면 ③·⑤) ───────────────────

// 요금제 목록 (앱 화면 ⑥) — active인 것만
function planList() {
  return { ok: true, plans: sheetTable(TAB.plans).rows
    .filter(r => String(r.status).trim() === "active")
    .map(r => ({
      plan_id: r.plan_id, plan_name: r.plan_name,
      hours: Number(r.hours), price: Number(r.price),
      extra_24h_price: Number(r.extra_24h_price)
    })) };
}

function lockerList() {
  return { ok: true, lockers: sheetTable(TAB.lockers).rows.map(r => ({
    locker_id: r.locker_id, locker_name: r.locker_name, location: r.location,
    total_slots: r.total_slots, available_slots: r.available_slots, status: r.status
  })) };
}

// 우산 현황 — locker_id를 주면 그 보관함만
function status(e) {
  const lockerId = e.parameter.locker_id;
  let rows = sheetTable(TAB.umbrellas).rows
    .filter(r => !isNaN(parseInt(r.slot_no)));            // 형식 안 맞는 행 제외
  if (lockerId) rows = rows.filter(r => String(r.locker_id) === String(lockerId));
  return { ok: true, umbrellas: rows.map(r => ({
    umbrella_id: r.umbrella_id, locker_id: r.locker_id,
    slot_no: r.slot_no, status: r.status
  })) };
}

// 기존 앱 호환용 — 이전 백엔드의 getUmbrella와 같은 모양으로 응답합니다.
//   ⚠ 이전 백엔드는 열을 한 칸씩 밀려 읽어서 status에 슬롯번호가, location에
//     상태값이 들어갔어요. 앱이 그 모양을 그대로 쓰고 있을 수 있으니 legacy 키는
//     건드리지 않고, 뜻이 분명한 키(slot_no·umbrella_status)를 함께 넣어줍니다.
//     앱을 고칠 때 새 키로 갈아타면 됩니다.
function getUmbrella(e) {
  const id = e.parameter.umbrella_id || e.parameter.umbrellaId;
  if (!id) return { ok: false, error: "우산 번호가 없습니다." };
  const row = findRow(TAB.umbrellas, "umbrella_id", id);
  if (!row) return { ok: false, error: "등록되지 않은 우산입니다." };
  return { ok: true, umbrella: {
    umbrella_id: row.umbrella_id,
    locker:   row.locker_id,        // legacy
    status:   row.slot_no,          // legacy (실제로는 슬롯 번호!)
    location: row.status,           // legacy (실제로는 대여 상태!)
    locker_id: row.locker_id,       // 뜻이 분명한 새 키 ↓
    slot_no: row.slot_no,
    umbrella_status: row.status
  } };
}

// ─────────────────── 대여 (앱 화면 ④⑤⑥⑧) ───────────────────
// QR로 인식한 locker_id + 사용자가 고른 umbrella_id로 대여
// (umbrella_id 생략 시 그 보관함의 첫 available 우산 자동 배정)
function rent(e) {
  const userId = e.parameter.user_id;
  if (!userId) return { ok: false, error: "user_id가 필요해요" };
  if (!findRow(TAB.users, "user_id", userId))
    return { ok: false, error: "등록되지 않은 사용자: " + userId };
  if (findActiveRental(userId))
    return { ok: false, error: "이미 대여 중이에요 — 먼저 반납하세요" };

  const lockerId   = e.parameter.locker_id;
  const umbrellaId = e.parameter.umbrella_id;
  const candidates = sheetTable(TAB.umbrellas).rows.filter(r =>
    r.status === "available" && !isNaN(parseInt(r.slot_no)) &&
    (!lockerId   || String(r.locker_id)   === String(lockerId)) &&
    (!umbrellaId || String(r.umbrella_id) === String(umbrellaId)));

  if (!candidates.length) {
    if (umbrellaId) return { ok: false, error: umbrellaId + "는 지금 대여할 수 없어요" };
    return { ok: false, error: "이 보관함에는 빌릴 수 있는 우산이 없어요" };
  }
  const target = candidates[0];

  const plan  = findRow(TAB.plans, "plan_id", e.parameter.plan_id || DEFAULT_PLAN);
  const hours = plan ? Number(plan.hours) : 24;
  const price = plan ? Number(plan.price) : 2000;

  const now = new Date();
  const expected = new Date(now.getTime() + hours * 3600 * 1000);
  const rentalId = nextId(TAB.rentals, "rental_id", "R");

  appendByHeader(TAB.rentals, {                  // ① 대여 기록 (후불이라 pending)
    rental_id: rentalId, user_id: userId,
    umbrella_id: target.umbrella_id, locker_id: target.locker_id,
    slot_no: target.slot_no, plan_hours: hours, plan_price: price,
    rental_time: now, expected_return: expected,
    status: "active", payment_status: "pending"
  });
  updateRow(TAB.umbrellas, "umbrella_id", target.umbrella_id, {   // ② 우산 상태
    status: "rented", last_user_id: userId, last_check_time: now,
    total_rentals: Number(target.total_rentals || 0) + 1
  });
  recountAvailableSlots(target.locker_id);       // ③ 보관함 빈 슬롯(실제 개수로 다시 셈)
  // ④ ESP32 슬롯 열림 (그 보관함 보드만 가져갑니다 · 보드가 없으면 큐에 남았다 정리됨)
  pushCommand("RENT", target.locker_id, target.slot_no);

  return { ok: true, rental_id: rentalId, umbrella_id: target.umbrella_id,
           locker_id: target.locker_id, slot_no: Number(target.slot_no),
           plan_hours: hours, plan_price: price, expected_return: expected };
}

// ─────────────── 반납 + 후불 정산 (앱 화면 ⑨⑩) ───────────────
// 실제 이용시간 계산 → 요금 = 요금제 가격 + 초과 24시간당 extra_24h_price
// → payments에 가상 결제 기록(paid) 생성 → rentals.payment_status=paid
function returnUmbrella(e) {
  const userId = e.parameter.user_id;
  if (!userId) return { ok: false, error: "user_id가 필요해요" };

  const rental = findActiveRental(userId);
  if (!rental) return { ok: false, error: "대여 중인 우산이 없어요" };

  const now = new Date();
  const rentedAt = rental.rental_time ? new Date(rental.rental_time) : now;
  const durationMin = Math.max(0, Math.round((now - rentedAt) / 60000));

  // ── 요금 계산 (후불) ──
  const planHours = Number(rental.plan_hours) || 24;
  const planPrice = Number(rental.plan_price) || 2000;
  const plan = sheetTable(TAB.plans).rows.find(r => Number(r.hours) === planHours);
  const extra24h = plan ? Number(plan.extra_24h_price) || DEFAULT_EXTRA_24H
                        : DEFAULT_EXTRA_24H;
  const overMin = Math.max(0, durationMin - planHours * 60);
  const extraBlocks = Math.ceil(overMin / (24 * 60));     // 초과분은 24시간 단위 올림
  const amount = planPrice + extraBlocks * extra24h;

  // ── ① 대여 종료 ──
  updateRow(TAB.rentals, "rental_id", rental.rental_id, {
    return_time: now, duration_min: durationMin,
    status: "returned", payment_status: "paid"
  });
  // ── ② 우산·보관함 복구 ──
  updateRow(TAB.umbrellas, "umbrella_id", rental.umbrella_id, {
    status: "available", last_check_time: now
  });
  recountAvailableSlots(rental.locker_id);
  // ── ③ 가상 결제 기록 ──
  const user = findRow(TAB.users, "user_id", userId);
  const paymentId = nextId(TAB.payments, "payment_id", "P");
  appendByHeader(TAB.payments, {
    payment_id: paymentId, rental_id: rental.rental_id, user_id: userId,
    amount: amount, method: "계좌 자동출금(가상)",
    account_masked: (user && user.account_masked) || "미등록",
    payment_time: now, status: "paid"
  });
  // ── ④ ESP32 슬롯 열림 ──
  pushCommand("RETURN", rental.locker_id, rental.slot_no);

  return { ok: true, rental_id: rental.rental_id, slot_no: Number(rental.slot_no),
           duration_min: durationMin, plan_price: planPrice,
           extra_charge: extraBlocks * extra24h, amount: amount,
           payment_id: paymentId,
           account_masked: (user && user.account_masked) || "미등록" };
}

// 내역 확인 (앱 화면 ⑩ '내역') — 이용 기록 + 결제 기록
function history(e) {
  const userId = e.parameter.user_id;
  if (!userId) return { ok: false, error: "user_id가 필요해요" };
  const rentals = sheetTable(TAB.rentals).rows
    .filter(r => String(r.user_id) === String(userId))
    .map(r => ({ rental_id: r.rental_id, umbrella_id: r.umbrella_id,
                 locker_id: r.locker_id, rental_time: r.rental_time,
                 return_time: r.return_time, duration_min: r.duration_min,
                 status: r.status, payment_status: r.payment_status }))
    .reverse();                                  // 최신이 위로
  const payments = sheetTable(TAB.payments).rows
    .filter(r => String(r.user_id) === String(userId))
    .map(r => ({ payment_id: r.payment_id, rental_id: r.rental_id,
                 amount: r.amount, method: r.method,
                 account_masked: r.account_masked,
                 payment_time: r.payment_time, status: r.status }))
    .reverse();
  return { ok: true, rentals: rentals, payments: payments };
}

// ─────────────────── ESP32 연동 (선택 사항) ───────────────────

// 슬롯별 우산 유무(p1~p4) 보고 — 30초마다 보드가 보냅니다.
// ★ 단순히 시간만 찍지 않고 "시트가 현실과 맞는지" 대조합니다.
//   빌린 사람이 없는데 rented로 남은 우산(테스트 잔재)이 실제로 꽂혀 있으면
//   available로 되돌리고, 실제로 없는데 available이면 알려줍니다.
//   available_slots도 매번 실제 개수로 다시 세서 어긋남이 쌓이지 않게 합니다.
function report(e) {
  const lockerId = e.parameter.locker_id;      // 없으면 첫 번째로 찾은 우산 (보드 1대 기준)
  const um = sheetTable(TAB.umbrellas);
  const rentals = sheetTable(TAB.rentals).rows;
  const now = new Date();
  let updated = 0, healed = 0;
  const mismatch = [];

  for (let slot = 1; slot <= 4; slot++) {
    const p = e.parameter["p" + slot];
    if (p === undefined) continue;
    const row = um.rows.find(r => parseInt(r.slot_no) === slot &&
      (!lockerId || String(r.locker_id) === String(lockerId)));
    if (!row) continue;

    const present = String(p) === "1";
    const changes = { last_check_time: now };
    const active = rentals.find(r => String(r.umbrella_id) === String(row.umbrella_id) &&
                                     r.status === "active");

    if (present && row.status === "rented" && !active) {
      changes.status = "available";             // 빌린 사람이 없는 고아 상태 → 복구
      changes.last_user_id = "";
      healed++;
    } else if (!present && row.status === "available") {
      mismatch.push(row.umbrella_id);           // 시트엔 있는데 실제로는 빈 칸
    }
    updateRow(TAB.umbrellas, "umbrella_id", row.umbrella_id, changes);
    updated++;
  }

  if (lockerId) recountAvailableSlots(lockerId);
  return { ok: true, updated: updated, healed: healed, mismatch: mismatch };
}

// 반납 실패: 보드가 "문은 열었는데 우산이 안 들어왔어요"라고 알릴 때 —
// 방금 처리한 반납을 되돌립니다(다시 대여 중 + 가상 결제 취소).
// 이게 없으면 우산은 손에 있는데 시트에는 반납 완료로 남아, 다음 사람이
// 그 칸을 빌렸을 때 빈 칸이 열립니다.
function returnFailed(e) {
  const slot = parseInt(e.parameter.slot);
  const lockerId = e.parameter.locker_id;
  const candidates = sheetTable(TAB.rentals).rows.filter(r =>
    parseInt(r.slot_no) === slot && r.status === "returned" &&
    (!lockerId || String(r.locker_id) === String(lockerId)));
  if (!candidates.length)
    return { ok: false, error: "슬롯 " + slot + "의 되돌릴 반납 기록이 없어요" };

  candidates.sort(function(x, y) {                    // 가장 최근에 반납된 것부터
    return new Date(x.return_time || 0) - new Date(y.return_time || 0);
  });
  const rental = candidates[candidates.length - 1];
  updateRow(TAB.rentals, "rental_id", rental.rental_id, {
    return_time: "", duration_min: "", status: "active", payment_status: "pending"
  });
  updateRow(TAB.umbrellas, "umbrella_id", rental.umbrella_id, {
    status: "rented", last_check_time: new Date()
  });
  // 이 반납으로 만들어진 가상 결제 기록도 취소 표시
  const pay = sheetTable(TAB.payments).rows
    .filter(r => String(r.rental_id) === String(rental.rental_id)).pop();
  if (pay) updateRow(TAB.payments, "payment_id", pay.payment_id, { status: "canceled" });

  recountAvailableSlots(rental.locker_id);
  return { ok: true, rental_id: rental.rental_id, reverted: true };
}

// 대여 취소: ESP32가 "우산이 안 빠졌어요"라고 알릴 때 — DB를 대여 전으로 되돌림
function cancelRental(e) {
  const slot = parseInt(e.parameter.slot);
  const lockerId = e.parameter.locker_id;      // 슬롯 번호는 보관함마다 겹치니 함께 비교!
  const rental = sheetTable(TAB.rentals).rows.find(r =>
    parseInt(r.slot_no) === slot && r.status === "active" &&
    (!lockerId || String(r.locker_id) === String(lockerId)));
  if (!rental) return { ok: false, error: "슬롯 " + slot + "의 대여 중 기록이 없어요" };

  const now = new Date();
  updateRow(TAB.rentals, "rental_id", rental.rental_id,
            { status: "canceled", return_time: now, duration_min: 0,
              payment_status: "canceled" });
  updateRow(TAB.umbrellas, "umbrella_id", rental.umbrella_id,
            { status: "available", last_check_time: now });
  recountAvailableSlots(rental.locker_id);
  return { ok: true, rental_id: rental.rental_id, canceled: true };
}

// ─────────────────── 명령 큐 (ESP32 ↔ 시트) ───────────────────
// 명령을 한 칸(A1)에 덮어쓰면, 폴링(3초) 사이에 두 명령이 겹칠 때 앞의 명령이
// 사라져 문이 안 열립니다. 그래서 줄 단위로 쌓고 오래된 것부터 꺼내요(FIFO).
//
// ★ 명령에는 보관함 번호를 함께 넣습니다 → "RENT:L001:2"
//   슬롯 번호(1~4)는 보관함마다 겹치기 때문에, 번호만 보내면 L002에서 빌린
//   명령을 L001 보드가 집어가 엉뚱한 칸이 열립니다. 보드는 자기 locker_id와
//   맞는 명령만 꺼내가고, 남의 명령은 큐에 그대로 남겨둡니다.
function commandSheet() {
  const sh = ensureSheetTab(TAB.commands);
  const a1 = String(sh.getRange(1, 1).getValue()).trim();
  const b1 = String(sh.getRange(1, 2).getValue()).trim();
  if (a1 !== "command" || b1 !== "locker_id") {
    sh.clear();            // 예전 형식(A1 한 칸·2열) 잔재 정리 — 대기 명령은 어차피 일회용
    sh.appendRow(["command", "locker_id", "pushed_at"]);
  }
  return sh;
}
function pushCommand(verb, lockerId, slotNo) {
  commandSheet().appendRow(
    [verb + ":" + lockerId + ":" + slotNo, lockerId, new Date()]);
}

// 이 보관함 앞으로 온 가장 오래된 명령을 꺼내면서 지웁니다(같은 명령 반복 방지).
// locker_id를 안 주면(브라우저 테스트 등) 맨 앞 명령을 그냥 꺼냅니다.
function popCommand(lockerId) {
  const sh = commandSheet();
  const last = sh.getLastRow();
  if (last < 2) return "";                       // 헤더만 있으면 대기 명령 없음
  const rows = sh.getRange(2, 1, last - 1, 2).getValues();
  for (let i = 0; i < rows.length; i++) {
    const cmd   = String(rows[i][0] || "").trim();
    if (!cmd) continue;
    const owner = String(rows[i][1] || "").trim() || commandLocker(cmd);
    // 주인이 없는 옛 형식("RENT:2")은 누구든 가져갈 수 있게 둡니다(하위 호환).
    if (lockerId && owner && owner !== String(lockerId)) continue;
    sh.deleteRow(i + 2);
    return cmd;
  }
  return "";
}

// "RENT:L001:2" → "L001"   /   "RENT:2" → ""
function commandLocker(cmd) {
  const parts = String(cmd).split(":");
  return parts.length >= 3 ? parts[1].trim() : "";
}
function ensureSheetTab(name) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  return ss.getSheetByName(name) || ss.insertSheet(name);
}

// ─────────── 새 시트 초기 설정 (스크립트 편집기에서 직접 실행) ───────────
// 빈 시트에 필요한 탭·제목 행·기본 데이터를 한 번에 만듭니다.
// 이미 있는 탭과 데이터는 건드리지 않으므로 여러 번 실행해도 안전합니다.
function setupSheets() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const defs = [
    [TAB.users,
     ["user_id","name","phone","email","created_at","status","account_masked"], []],
    [TAB.lockers,
     ["locker_id","locker_name","location","latitude","longitude",
      "total_slots","available_slots","status","last_update"],
     [["L001","도서관 앞 보관함","학교 도서관 앞","","",4,4,"active",new Date()]]],
    [TAB.umbrellas,
     ["umbrella_id","locker_id","slot_no","status","last_check_time",
      "total_rentals","last_user_id"],
     [["UB001","L001",1,"available",new Date(),0,""],
      ["UB002","L001",2,"available",new Date(),0,""],
      ["UB003","L001",3,"available",new Date(),0,""],
      ["UB004","L001",4,"available",new Date(),0,""]]],
    [TAB.rentals,
     ["rental_id","user_id","umbrella_id","locker_id","slot_no","plan_hours",
      "plan_price","rental_time","expected_return","return_time","duration_min",
      "status","payment_status"], []],
    [TAB.payments,
     ["payment_id","rental_id","user_id","amount","method","account_masked",
      "payment_time","status"], []],
    ["daily_stats",
     ["date","total_rentals","total_returns","active_rentals","popular_locker",
      "popular_umbrella","avg_duration_min"], []],
    [TAB.plans,
     ["plan_id","plan_name","hours","price","extra_24h_price","status"],
     [["P001","24시간",24,2000,1000,"active"],
      ["P002","48시간",48,3000,1000,"active"],
      ["P003","72시간",72,4000,1000,"active"]]]
  ];
  defs.forEach(function(def) {
    const name = def[0], headers = def[1], rows = def[2];
    let sh = ss.getSheetByName(name);
    if (!sh) sh = ss.insertSheet(name);
    if (sh.getLastRow() === 0) {           // 완전히 빈 탭에만 채움
      sh.appendRow(headers);
      rows.forEach(function(r) { sh.appendRow(r); });
    }
  });
  ensureSheetTab(TAB.commands);
}

// ─────────── 박람회용: 데모 초기화 (스크립트 편집기에서 직접 실행) ───────────
// rentals·payments를 비우고 모든 우산을 available로 되돌립니다. 부스 리셋용!
function resetDemo() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  [TAB.rentals, TAB.payments].forEach(name => {
    const sh = ss.getSheetByName(name);
    if (sh && sh.getLastRow() > 1)
      sh.getRange(2, 1, sh.getLastRow() - 1, sh.getLastColumn()).clearContent();
  });
  const um = sheetTable(TAB.umbrellas);
  um.rows.forEach(r => {
    if (!isNaN(parseInt(r.slot_no)))
      updateRow(TAB.umbrellas, "umbrella_id", r.umbrella_id,
                { status: "available", last_user_id: "" });
  });
  // 보관함 빈 슬롯 수를 전체 슬롯 수로 복구
  sheetTable(TAB.lockers).rows.forEach(r =>
    updateRow(TAB.lockers, "locker_id", r.locker_id,
              { available_slots: r.total_slots, last_update: new Date() }));
  ensureSheetTab(TAB.commands).clear();          // 남은 명령 큐도 비우기
}

// ─────────────────── 시트 헬퍼 (제목 행 기준) ───────────────────
// 열 순서가 바뀌어도 동작하도록 항상 "제목 행 이름"으로 읽고 씁니다.

function sheetTable(name) {
  const sh = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(name);
  if (!sh) throw "시트 탭이 없어요: " + name + " (TAB 설정 확인!)";
  const values = sh.getDataRange().getValues();
  const headers = values[0].map(h => String(h).trim());
  const rows = values.slice(1).map((v, i) => {
    const obj = { _row: i + 2 };               // 실제 시트 행 번호
    headers.forEach((h, c) => obj[h] = v[c]);
    return obj;
  });
  return { sheet: sh, headers: headers, rows: rows };
}

function findRow(name, key, value) {
  return sheetTable(name).rows.find(r => String(r[key]) === String(value)) || null;
}

function findActiveRental(userId) {
  return sheetTable(TAB.rentals).rows.find(r =>
    String(r.user_id) === String(userId) && r.status === "active") || null;
}

function updateRow(name, key, value, changes) {
  const t = sheetTable(name);
  const row = t.rows.find(r => String(r[key]) === String(value));
  if (!row) return false;
  for (const col in changes) {
    const c = t.headers.indexOf(col);
    if (c >= 0) t.sheet.getRange(row._row, c + 1).setValue(changes[col]);
  }
  return true;
}

function appendByHeader(name, obj) {
  const t = sheetTable(name);
  t.sheet.appendRow(t.headers.map(h => (h in obj) ? obj[h] : ""));
}

// 제목 행에 열이 없으면 맨 오른쪽에 추가 (예: users의 account_masked)
function ensureColumn(name, colName) {
  const t = sheetTable(name);
  if (t.headers.indexOf(colName) >= 0) return;
  t.sheet.getRange(1, t.headers.length + 1).setValue(colName);
}

// 빈 슬롯 수는 더하기·빼기로 관리하면 취소·오류가 한 번 날 때마다 어긋나서
// 앱 목록의 "3/4 대여 가능"이 실제와 달라집니다. 그래서 매번 실제로 다시 셉니다.
function recountAvailableSlots(lockerId) {
  if (!lockerId) return;
  if (!findRow(TAB.lockers, "locker_id", lockerId)) return;
  const free = sheetTable(TAB.umbrellas).rows.filter(r =>
    String(r.locker_id) === String(lockerId) &&
    !isNaN(parseInt(r.slot_no)) && r.status === "available").length;
  updateRow(TAB.lockers, "locker_id", lockerId,
            { available_slots: free, last_update: new Date() });
}

function nextId(name, key, prefix) {          // R001 → R002 …
  const rows = sheetTable(name).rows;
  let max = 0;
  rows.forEach(r => {
    const n = parseInt(String(r[key] || "").replace(prefix, ""));
    if (!isNaN(n) && n > max) max = n;
  });
  return prefix + String(max + 1).padStart(3, "0");
}

// ─────────────────── 응답 헬퍼 ───────────────────
function json(obj) {
  // 기존 앱이 success/message를 보고 있어도 동작하도록 같은 뜻의 키를 함께 넣어줍니다.
  if (obj && typeof obj === "object") {
    if ("ok" in obj && !("success" in obj)) obj.success = obj.ok;
    if ("error" in obj && !("message" in obj)) obj.message = obj.error;
  }
  return ContentService.createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}
function text(s) {
  return ContentService.createTextOutput(s);
}
