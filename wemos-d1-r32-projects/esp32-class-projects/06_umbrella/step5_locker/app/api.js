/*
  SMART UMBRELLA — 앱에서 쓰는 서버 연동 코드 (화면별 함수 모음)
  ----------------------------------------------------------------------
  이 파일 하나만 앱에 넣으면 모든 화면에서 UmbrellaAPI.xxx() 로 호출할 수 있어요.

  ★ 딱 한 줄만 바꾸세요 → SERVER_URL (구글 시트 '시트1' 탭의 /exec 주소)

  ※ 왜 전부 GET인가요?
    Apps Script는 브라우저의 "사전 확인 요청(preflight)"에 응답하지 않아요.
    fetch에 헤더를 붙이거나 JSON으로 POST하면 CORS 오류가 납니다.
    그래서 여기서는 주소 뒤에 값을 붙이는 GET만 씁니다 — 가장 안전해요.
*/

const SERVER_URL = "여기에_시트1_탭의_exec_주소_붙여넣기";

// 로그인 정보는 폰에 저장 (개인정보 최초 1회만 입력하는 UX)
const STORE = { userId: "umbrella_user_id", userName: "umbrella_user_name" };

// ── 공통 호출 함수 ──────────────────────────────────────
async function callApi(action, params = {}) {
  const q = new URLSearchParams({ action, ...params });
  const res = await fetch(`${SERVER_URL}?${q}`);       // 헤더 없음 = CORS 안전
  if (!res.ok) throw new Error(`서버 응답 오류 (${res.status})`);

  const data = await res.json();
  if (data.ok === false) throw new Error(data.error || "알 수 없는 오류");
  return data;
}

const UmbrellaAPI = {
  // ── 화면 ② 개인정보 입력 ───────────────────────────
  // 같은 전화번호가 이미 있으면 기존 user_id를 그대로 돌려줍니다(중복 가입 없음)
  async register(name, phone, email = "") {
    const r = await callApi("register", { name, phone, email });
    localStorage.setItem(STORE.userId, r.user_id);      // 다음부터 자동 입력
    localStorage.setItem(STORE.userName, r.name || name);
    return r;                                            // { user_id, name, existing }
  },

  // ── 화면 ⑦ 결제 수단(계좌) 등록 ─────────────────────
  // 계좌번호는 마스킹해서 저장해요 (123-456-789012 → 123-***-9012)
  async registerAccount(account) {
    return callApi("register_account", { user_id: this.getUserId(), account });
  },                                                     // { account_masked }

  // ── 화면 ③ 보관함 찾기 ──────────────────────────────
  async getLockers() {
    const r = await callApi("lockers");
    return r.lockers;      // [{ locker_id, locker_name, location, total_slots, available_slots, status }]
  },

  // ── 화면 ⑤ 우산 선택 ────────────────────────────────
  // lockerId를 주면 그 보관함 우산만, 안 주면 전체
  async getUmbrellas(lockerId) {
    const r = await callApi("status", lockerId ? { locker_id: lockerId } : {});
    return r.umbrellas;    // [{ umbrella_id, locker_id, slot_no, status }]
  },

  // ── 화면 ⑥ 요금제 선택 ──────────────────────────────
  async getPlans() {
    const r = await callApi("plans");
    return r.plans;        // [{ plan_id, plan_name, hours, price, extra_24h_price }]
  },

  // ── 화면 ⑧ 대여 (④QR + ⑤우산 + ⑥요금제를 합쳐서 호출) ──
  async rent(lockerId, umbrellaId, planId) {
    return callApi("rent", {
      user_id: this.getUserId(),
      locker_id: lockerId,
      umbrella_id: umbrellaId,
      plan_id: planId
    });   // { rental_id, umbrella_id, locker_id, slot_no, plan_hours, plan_price, expected_return }
  },

  // ── 화면 ⑨⑩ 반납 + 후불 결제 (한 번에 처리됨) ────────
  async returnUmbrella() {
    return callApi("return", { user_id: this.getUserId() });
    // { rental_id, slot_no, duration_min, plan_price, extra_charge, amount,
    //   payment_id, account_masked }
  },

  // ── 내역 화면 ───────────────────────────────────────
  async getHistory() {
    return callApi("history", { user_id: this.getUserId() });
    // { rentals: [...], payments: [...] }  — 최신이 위
  },

  // ── 로그인 상태 (화면 ① 홈에서 분기용) ───────────────
  getUserId()   { return localStorage.getItem(STORE.userId); },
  getUserName() { return localStorage.getItem(STORE.userName); },
  isRegistered() { return !!this.getUserId(); },
  logout() { localStorage.removeItem(STORE.userId);
             localStorage.removeItem(STORE.userName); }
};
