// mood_lamp.gs 를 PC에서 돌려보는 테스트 — 구글 시트·캐시·잠금을 흉내 낸다.
//   node apps_script/test/gs_test.js
const fs = require('fs');
const path = require('path');
const vm = require('vm');

// ── 구글 서비스 흉내 ──────────────────────────────────────────────
let now = Date.parse('2026-10-05T09:00:00+09:00');
class FakeDate extends Date {
  constructor(...a) { super(...(a.length ? a : [now])); }
  static now() { return now; }
}

function makeSheet(name) {
  const rows = [];
  const sheet = {
    name, rows,
    clear() { rows.length = 0; },
    appendRow(r) { rows.push(r.slice()); },
    getLastRow() { return rows.length; },
    setFrozenRows() {},
    getRange(r, c, nr = 1, nc = 1) {
      return {
        setValues(v) { for (let i = 0; i < nr; i++) { rows[r - 1 + i] = rows[r - 1 + i] || []; for (let j = 0; j < nc; j++) rows[r - 1 + i][c - 1 + j] = v[i][j]; } return this; },
        getValues() { const out = []; for (let i = 0; i < nr; i++) { const row = rows[r - 1 + i] || []; out.push(Array.from({ length: nc }, (_, j) => row[c - 1 + j] === undefined ? '' : row[c - 1 + j])); } return out; },
        setFontWeight() { return this; },
      };
    },
  };
  return sheet;
}
const sheets = {};
const cacheStore = new Map();
let cacheWrites = 0;
const ctx = {
  Date: FakeDate, JSON, Math, Number, String, isNaN,
  SpreadsheetApp: { getActive: () => ({ getSheetByName: (n) => sheets[n] || null, insertSheet: (n) => (sheets[n] = makeSheet(n)) }) },
  CacheService: { getScriptCache: () => ({
    get: (k) => (cacheStore.has(k) ? cacheStore.get(k) : null),
    put: (k, v) => { cacheStore.set(k, v); cacheWrites++; },
    getAll: (ks) => Object.fromEntries(ks.filter((k) => cacheStore.has(k)).map((k) => [k, cacheStore.get(k)])),
  }) },
  LockService: { getScriptLock: () => ({ tryLock: () => true, releaseLock: () => {} }) },
  ContentService: { MimeType: { JSON: 'json' }, createTextOutput: (s) => ({ body: s, setMimeType() { return this; } }) },
};
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(path.join(__dirname, '..', 'mood_lamp.gs'), 'utf8'), ctx);

const get = (params) => JSON.parse(ctx.doGet({ parameter: params }).body);
let pass = 0, fail = 0;
const check = (ok, what) => { console.log(`  ${ok ? '✅' : '❌'} ${what}`); ok ? pass++ : fail++; };
const section = (t) => console.log(`\n▶ ${t}`);

// ── 시나리오 ─────────────────────────────────────────────────────
console.log('════ mood_lamp.gs 중계 서버 테스트 (12팀 · 시트 1개) ════');
ctx.setupSheets();

section('1. 처음 설정');
check(sheets.state.rows.length === 13 && sheets.state.rows[12][0] === 'TEAM12', 'state 탭: 제목 + TEAM01~TEAM12 12행');
check(sheets.log.rows[0][0] === '시각', 'log 탭 생성');

section('2. 팀 이름 검사');
check(get({ mode: 'set', team: 'TEAM13', cmd: 'ON' }).ok === false, 'TEAM13 → 거절');
check(get({ mode: 'set', team: 'team3', cmd: 'ON' }).team === 'TEAM03', "'team3' → TEAM03 으로 정리");
get({ mode: 'next', team: 'TEAM03' });
check(get({ mode: 'set', team: 'TEAM01', cmd: 'DANCE' }).ok === false, '모르는 명령 → 거절');
check(get({ mode: 'oops' }).ok === false, '모르는 mode → 안내');

section('3. 명령 대기열 — 팀끼리 섞이지 않음');
get({ mode: 'set', team: 'TEAM01', cmd: 'ON' });
get({ mode: 'set', team: 'TEAM02', cmd: 'COLOR', value: 'blue' });
check(get({ mode: 'next', team: 'TEAM02' }).value === 'blue', 'TEAM02는 자기 명령(COLOR blue)만 받음');
check(get({ mode: 'next', team: 'TEAM01' }).cmd === 'ON', 'TEAM01은 ON');
check(get({ mode: 'next', team: 'TEAM01' }).cmd === '', '꺼낸 명령은 다시 안 나옴');

section('4. 슬라이더 연타 — 마지막 값만');
for (const v of [10, 30, 70, 150]) get({ mode: 'set', team: 'TEAM05', cmd: 'BRIGHT', value: String(v) });
get({ mode: 'set', team: 'TEAM05', cmd: 'ON' });
get({ mode: 'set', team: 'TEAM05', cmd: 'OFF' });
const a = get({ mode: 'next', team: 'TEAM05' }), b = get({ mode: 'next', team: 'TEAM05' }), c = get({ mode: 'next', team: 'TEAM05' });
check(a.cmd === 'BRIGHT' && a.value === '100', 'BRIGHT 4번 → 마지막 하나, 150은 100으로 제한');
check(b.cmd === 'OFF' && c.cmd === '', 'ON 뒤 OFF → OFF 하나만');

section('5. 자동 설정(CONFIG)');
get({ mode: 'set', team: 'TEAM07', cmd: 'CONFIG', dark: '35', off: '60' });
const cfg = get({ mode: 'next', team: 'TEAM07' });
check(cfg.cmd === 'CONFIG' && cfg.dark === 35 && cfg.off === 60, 'dark=35 · off=60 그대로 전달');
get({ mode: 'set', team: 'TEAM07', cmd: 'CONFIG', dark: '999', off: '1' });
const cfg2 = get({ mode: 'next', team: 'TEAM07' });
check(cfg2.dark === 100 && cfg2.off === 10, '범위 밖 값은 제한 (dark 0~100, off 10~3600초)');

section('6. 10분 지난 명령은 버림');
get({ mode: 'set', team: 'TEAM08', cmd: 'ON' });
now += 11 * 60 * 1000;
check(get({ mode: 'next', team: 'TEAM08' }).cmd === '', '11분 뒤 → 명령 없음 (늦게 켜진 보드가 엉뚱하게 켜지지 않게)');

section('7. 상태 보고 → 앱 읽기');
get({ mode: 'report', team: 'TEAM01', type: 'bulb', power: 'ON', auto: '1', light: '28', motion: '1', dark: '30', off: '300' });
let s = get({ mode: 'state', team: 'TEAM01' });
check(s.online === true && s.type === 'bulb' && s.power === 'ON' && s.light === 28, '보고한 값 그대로 + online');
check(sheets.state.rows[1][3] === 'ON', 'state 탭 TEAM01 행에 기록');
now += 45 * 1000;
check(get({ mode: 'state', team: 'TEAM01' }).online === false, '45초 동안 보고 없으면 online=false');
check(get({ mode: 'state', team: 'TEAM04' }).online === false, '한 번도 보고 안 한 팀 → online=false');

section('8. 캐시가 사라져도 시트에서 복구');
cacheStore.delete('s_TEAM01');
s = get({ mode: 'state', team: 'TEAM01' });
check(s.power === 'ON' && s.type === 'bulb', '시트 state 행에서 다시 읽음');

section('9. 12팀 한눈에');
get({ mode: 'report', team: 'TEAM02', type: 'neopixel', power: 'OFF', color: 'warm', bright: '60' });
const t = get({ mode: 'teams' });
check(t.teams.length === 12, '12팀 목록');
check(t.teams[1].online === true && t.teams[1].type === 'neopixel', 'TEAM02 online · neopixel');

section('10. 시트 쓰기 줄이기');
const before = sheets.state.rows[3] ? JSON.stringify(sheets.state.rows[3]) : '';
get({ mode: 'report', team: 'TEAM03', type: 'bulb', power: 'OFF', light: '50' });
const w1 = JSON.stringify(sheets.state.rows[3]);
now += 5000; get({ mode: 'report', team: 'TEAM03', type: 'bulb', power: 'OFF', light: '52' });
check(JSON.stringify(sheets.state.rows[3]) === w1 && w1 !== before, '조도만 조금 바뀐 보고는 시트에 다시 안 씀 (캐시만)');
now += 5000; get({ mode: 'report', team: 'TEAM03', type: 'bulb', power: 'ON', light: '52' });
check(sheets.state.rows[3][3] === 'ON', '켜짐/꺼짐이 바뀌면 바로 시트에 기록');

section('11. 기록에 재실 정보 없음');
check(sheets.log.rows.every((r) => !String(r.join(',')).includes('motion')), 'log 탭에는 명령만 — 인체감지 기록 없음');

console.log(`\n결과: 통과 ${pass} / 실패 ${fail}`);
process.exit(fail ? 1 : 0);
