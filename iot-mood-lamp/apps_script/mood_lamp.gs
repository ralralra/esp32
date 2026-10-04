/**
 * IoT 무드등 — 중계 서버 (Google Apps Script)
 *
 * 구글 시트 1개 + 이 스크립트 1개로 12팀(TEAM01~TEAM12)이 함께 씁니다.
 * 앱과 ESP32는 서로 직접 통신할 수 없어서(HTTPS 앱 ↔ 교실 와이파이 안의 ESP32) 이 서버를 거칩니다.
 *
 *   앱  → ?mode=set&team=TEAM01&cmd=ON          명령 넣기
 *   앱  → ?mode=state&team=TEAM01               팀 상태 읽기
 *   앱  → ?mode=teams                           12팀 연결 상태 한눈에
 *   ESP → ?mode=next&team=TEAM01                내 팀 명령 하나 꺼내기 (2초마다)
 *   ESP → ?mode=report&team=TEAM01&type=..&...  내 상태 보고 (바뀔 때 + 30초마다)
 *
 * 빠르게 바뀌는 값(명령 대기열·현재 상태)은 캐시(CacheService)에 두고,
 * 시트는 사람이 보는 기록용으로만 씁니다 — 12대가 2초마다 물어도 느려지지 않게.
 *
 * 처음 한 번: 편집기에서 setupSheets 실행 → 배포 → 새 배포 → 웹 앱
 *   (실행: 나 / 액세스: 모든 사용자) → 끝이 /exec 인 URL을 앱과 ESP32 코드에 넣기
 */

const TEAM_COUNT = 12;
const ONLINE_SEC = 40;              // 마지막 보고가 이보다 오래되면 '연결 끊김'
const QUEUE_TTL_SEC = 600;          // 10분 동안 아무도 안 가져간 명령은 버림
const STATE_TTL_SEC = 21600;        // 캐시 최대 보관 6시간 (사라지면 시트에서 다시 읽음)
const COMMANDS = ['ON', 'OFF', 'AUTO', 'COLOR', 'BRIGHT', 'CONFIG'];
const STATE_KEYS = ['type', 'power', 'auto', 'color', 'bright', 'light', 'motion', 'dark', 'off'];

// ── 진입점 ─────────────────────────────────────────────────────────
function doGet(e) {
  const p = (e && e.parameter) || {};
  try {
    switch (p.mode) {
      case 'set':    return json_(setCommand_(p));
      case 'next':   return json_(nextCommand_(p));
      case 'report': return json_(report_(p));
      case 'state':  return json_(state_(p));
      case 'teams':  return json_(teams_());
      default:       return json_({ ok: false, error: 'mode는 set · next · report · state · teams 중 하나' });
    }
  } catch (err) {
    return json_({ ok: false, error: String(err && err.message || err) });
  }
}

// ── 앱 → 명령 넣기 ────────────────────────────────────────────────
function setCommand_(p) {
  const team = team_(p.team);
  const cmd = String(p.cmd || '').toUpperCase();
  if (COMMANDS.indexOf(cmd) < 0) throw new Error('cmd는 ' + COMMANDS.join(' · ') + ' 중 하나');

  const item = { cmd: cmd, value: String(p.value || ''), t: Date.now() };
  if (cmd === 'BRIGHT') item.value = String(clamp_(Number(p.value), 0, 100));
  if (cmd === 'CONFIG') {
    item.dark = clamp_(Number(p.dark), 0, 100);
    item.off = clamp_(Number(p.off), 10, 3600);
  }

  withLock_(function () {
    const q = readQueue_(team).filter(function (old) { return !sameGroup_(old.cmd, cmd); });
    q.push(item);                                   // 같은 종류는 마지막 것만 남김 (슬라이더 연타 대비)
    CacheService.getScriptCache().put('q_' + team, JSON.stringify(q), QUEUE_TTL_SEC);
  });
  log_(team, 'set', cmd, item.value + (cmd === 'CONFIG' ? ' dark=' + item.dark + ' off=' + item.off : ''));
  return { ok: true, team: team, cmd: cmd };
}

// 켜기·끄기·자동은 서로 덮어쓰고, 색·밝기·설정은 각자 최신 하나만
function sameGroup_(a, b) {
  const power = ['ON', 'OFF', 'AUTO'];
  if (power.indexOf(a) >= 0 && power.indexOf(b) >= 0) return true;
  return a === b;
}

// ── ESP32 → 명령 하나 꺼내기 ──────────────────────────────────────
function nextCommand_(p) {
  const team = team_(p.team);
  let item = null;
  withLock_(function () {
    const q = readQueue_(team);
    if (q.length) {
      item = q.shift();
      CacheService.getScriptCache().put('q_' + team, JSON.stringify(q), QUEUE_TTL_SEC);
    }
  });
  if (!item) return { cmd: '' };
  const out = { cmd: item.cmd, value: item.value };
  if (item.cmd === 'CONFIG') { out.dark = item.dark; out.off = item.off; }
  return out;
}

function readQueue_(team) {
  const raw = CacheService.getScriptCache().get('q_' + team);
  if (!raw) return [];
  const limit = Date.now() - QUEUE_TTL_SEC * 1000;
  return JSON.parse(raw).filter(function (it) { return it.t >= limit; });
}

// ── ESP32 → 상태 보고 ─────────────────────────────────────────────
function report_(p) {
  const team = team_(p.team);
  const s = { team: team, updated: Date.now() };
  STATE_KEYS.forEach(function (k) {
    if (p[k] === undefined || p[k] === '') return;
    s[k] = /^-?\d+(\.\d+)?$/.test(p[k]) ? Number(p[k]) : String(p[k]);
  });
  const cache = CacheService.getScriptCache();
  const prev = cache.get('s_' + team);
  cache.put('s_' + team, JSON.stringify(s), STATE_TTL_SEC);
  // 시트는 바뀐 값이 있을 때만 (또는 1분에 한 번) 적어 쓰기 횟수를 줄인다
  if (!prev || changed_(JSON.parse(prev), s) || s.updated - JSON.parse(prev).updated > 60000) writeStateRow_(s);
  return { ok: true };
}

function changed_(a, b) {
  return ['type', 'power', 'auto', 'color', 'bright', 'dark', 'off'].some(function (k) { return a[k] !== b[k]; });
}

// ── 앱 → 상태 읽기 ────────────────────────────────────────────────
function state_(p) {
  const team = team_(p.team);
  let s = null;
  const raw = CacheService.getScriptCache().get('s_' + team);
  if (raw) s = JSON.parse(raw);
  else s = readStateRow_(team);                      // 캐시가 비었으면 시트에서
  if (!s) return { ok: true, team: team, online: false };
  s.ok = true;
  s.online = Date.now() - s.updated < ONLINE_SEC * 1000;
  s.age = Math.round((Date.now() - s.updated) / 1000);
  return s;
}

function teams_() {
  const keys = [];
  for (let i = 1; i <= TEAM_COUNT; i++) keys.push('s_' + teamName_(i));
  const all = CacheService.getScriptCache().getAll(keys);
  const list = [];
  for (let i = 1; i <= TEAM_COUNT; i++) {
    const name = teamName_(i);
    const s = all['s_' + name] ? JSON.parse(all['s_' + name]) : null;
    list.push({ team: name, online: !!s && Date.now() - s.updated < ONLINE_SEC * 1000, type: s ? s.type || '' : '' });
  }
  return { ok: true, teams: list };
}

// ── 시트 ────────────────────────────────────────────────────────
const STATE_HEADER = ['team', 'updated', 'type', 'power', 'auto', 'color', 'bright', 'light', 'motion', 'dark', 'off'];
const LOG_HEADER = ['시각', 'team', '종류', 'cmd', 'value'];

/** 처음 한 번 실행: state(12팀 행)와 log 탭을 만든다 */
function setupSheets() {
  const ss = SpreadsheetApp.getActive();
  let st = ss.getSheetByName('state') || ss.insertSheet('state');
  st.clear();
  st.getRange(1, 1, 1, STATE_HEADER.length).setValues([STATE_HEADER]).setFontWeight('bold');
  const rows = [];
  for (let i = 1; i <= TEAM_COUNT; i++) {
    const r = STATE_HEADER.map(function () { return ''; });
    r[0] = teamName_(i);
    rows.push(r);
  }
  st.getRange(2, 1, TEAM_COUNT, STATE_HEADER.length).setValues(rows);
  st.setFrozenRows(1);
  let lg = ss.getSheetByName('log') || ss.insertSheet('log');
  if (lg.getLastRow() === 0) lg.appendRow(LOG_HEADER);
  lg.setFrozenRows(1);
}

function writeStateRow_(s) {
  const st = SpreadsheetApp.getActive().getSheetByName('state');
  if (!st) return;
  const row = teamNumber_(s.team) + 1;
  const values = STATE_HEADER.map(function (k) {
    if (k === 'updated') return new Date(s.updated);
    return s[k] === undefined ? '' : s[k];
  });
  st.getRange(row, 1, 1, values.length).setValues([values]);
}

function readStateRow_(team) {
  const st = SpreadsheetApp.getActive().getSheetByName('state');
  if (!st) return null;
  const v = st.getRange(teamNumber_(team) + 1, 1, 1, STATE_HEADER.length).getValues()[0];
  if (!v[1]) return null;
  const s = {};
  STATE_HEADER.forEach(function (k, i) { if (v[i] !== '') s[k] = v[i]; });
  s.updated = new Date(v[1]).getTime();
  return s;
}

/** 명령 기록 — 인체감지(재실) 정보는 남기지 않는다 */
function log_(team, kind, cmd, value) {
  const lg = SpreadsheetApp.getActive().getSheetByName('log');
  if (lg) lg.appendRow([new Date(), team, kind, cmd, value]);
}

// ── 도구 ────────────────────────────────────────────────────────
function team_(raw) {
  const t = String(raw || '').toUpperCase().trim();
  const m = /^TEAM(\d{1,2})$/.exec(t);
  if (!m || Number(m[1]) < 1 || Number(m[1]) > TEAM_COUNT) throw new Error('team은 TEAM01 ~ TEAM' + pad_(TEAM_COUNT));
  return teamName_(Number(m[1]));
}
function teamName_(n) { return 'TEAM' + pad_(n); }
function teamNumber_(team) { return Number(team.slice(4)); }
function pad_(n) { return (n < 10 ? '0' : '') + n; }
function clamp_(x, lo, hi) { return isNaN(x) ? lo : Math.max(lo, Math.min(hi, Math.round(x))); }

function withLock_(fn) {
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(5000)) throw new Error('서버가 바빠요 — 잠시 후 다시');
  try { fn(); } finally { lock.releaseLock(); }
}

function json_(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(ContentService.MimeType.JSON);
}
