/**
 * 스마트 출석체크 — 중계 서버 (Google Apps Script) 완성판
 * ----------------------------------------------------
 * 앱(Cloud Run)과 ESP32가 서로 직접 통신할 수 없으므로,
 * 둘 다 접속 가능한 이 웹앱이 가운데에서 심부름을 한다.
 * 학생명단·출석기록 데이터베이스도 이 스크립트가 붙은 구글 시트에 저장된다.
 *
 * ── 구글 시트 구성 (없으면 처음 사용될 때 자동 생성) ──
 *   [학생명단]  A:UID  B:이름  C:학년  D:반  E:번호  F:등록일시
 *   [출석기록]  A:날짜  B:시각  C:교시  D:UID  E:이름  F:상태  G:리더기
 *   [교시설정]  A:교시  B:시작시각  C:지각기준   ← 지각 판정 기준 (수정해서 쓰세요)
 *   [설정]      A:항목  B:값                    ← 교사 비밀번호 (수정해서 쓰세요)
 *
 * ── 주고받는 말 (action 파라미터, 전부 GET) ──
 *   ▸ 앱이 부르는 것
 *   login    : 교사 비밀번호 확인. pin                                → JSON
 *   start    : 세션 시작. mode=attend|register, device, [name, grade, klass, number,
 *              period, lateAfter(HH:MM), timeout(초, 기본 30 — 등록은 최소 120 보장), pin] → JSON
 *   status   : 세션 상태/결과 조회. device                            → JSON
 *   cancel   : 세션 취소. device                                      → JSON
 *   students : 등록된 학생 목록. [pin]                                → JSON
 *   records  : 출석기록 조회. [date(yyyy-MM-dd, 기본 오늘), period, pin] → JSON
 *   attended : 해당 날짜·교시의 출석 현황(학생별 완료 여부). period, [date] → JSON
 *   periods  : 교시설정 목록                                          → JSON
 *   ▸ ESP32가 부르는 것 (한 줄 텍스트 응답)
 *   poll     : 할 일 조회. device        → "IDLE" | "ATTEND" | "REGISTER"
 *   tag      : 카드 태그 보고. device, uid → "OK,이름,상태,시각" 등
 *
 * ── 비밀번호 보호 ──
 *   [설정] 시트의 '비밀번호보호'가 ON이면 관리 기능(start·cancel·students·records)에
 *   pin이 필요합니다. OFF(기본값)면 pin 없이도 동작합니다.
 *   ESP32가 쓰는 poll·tag와 periods·attended는 언제나 pin 없이 동작합니다.
 */

var SHEET_STUDENTS = '학생명단';
var SHEET_LOG = '출석기록';
var SHEET_PERIODS = '교시설정';
var SHEET_CONFIG = '설정';

function doGet(e)  { return route(e); }
function doPost(e) { return route(e); }

function route(e) {
  var p = (e && e.parameter) || {};
  ensureSheets();   // 시트가 없으면 한 번에 만들어 둔다

  // 비밀번호 보호가 켜져 있으면 관리 기능은 pin을 확인한다
  if (PROTECTED_ACTIONS.indexOf(p.action) >= 0 && !checkPin(p.pin)) {
    return json({ ok: false, error: 'auth', message: '비밀번호가 필요합니다' });
  }

  switch (p.action) {
    case 'login':    return login(p);
    case 'start':    return startSession(p);
    case 'status':   return sessionStatus(p);
    case 'cancel':   return cancelSession(p);
    case 'students': return listStudents(p);
    case 'records':  return listRecords(p);
    case 'attended': return listAttended(p);
    case 'periods':  return listPeriods(p);
    case 'poll':     return devicePoll(p);
    case 'tag':      return deviceTag(p);
    default:         return text('ERR,unknown_action');
  }
}

// 비밀번호 보호가 켜졌을 때 pin이 필요한 기능들
// (poll·tag는 ESP32가 쓰므로, periods·attended·status는 화면 표시용이므로 제외)
var PROTECTED_ACTIONS = ['start', 'cancel', 'students', 'records'];

// ────────────────────────── 앱 쪽 ──────────────────────────

// 교사 비밀번호 확인 (앱의 로그인 화면용)
// 보호가 꺼져 있으면 언제나 통과 → 앱은 로그인 화면을 건너뛰면 된다
function login(p) {
  if (!isProtected()) return json({ ok: true, protected: false });
  if (checkPin(p.pin)) return json({ ok: true, protected: true });
  return json({ ok: false, protected: true, message: '비밀번호가 올바르지 않습니다' });
}

// 해당 날짜·교시의 출석 현황 — 등록된 학생 전체에 출석 여부를 표시해서 돌려준다
// 앱의 "출석 완료 / 미출석" 현황판에 쓴다
function listAttended(p) {
  var date = p.date || now('yyyy-MM-dd');
  var period = String(p.period || '');

  // 해당 날짜·교시의 출석기록을 UID로 찾아보기 쉽게 모은다
  var logRows = getSheet(SHEET_LOG, ['날짜', '시각', '교시', 'UID', '이름', '상태', '리더기'])
    .getDataRange().getValues();
  var done = {};
  for (var i = 1; i < logRows.length; i++) {
    if (formatCellDate(logRows[i][0]) !== date) continue;
    if (period && String(logRows[i][2]) !== period) continue;
    done[String(logRows[i][3]).toUpperCase()] = {
      time: formatCellTime(logRows[i][1]),
      status: String(logRows[i][5])
    };
  }

  // 등록된 학생 전체에 출석 여부를 붙인다
  var stuRows = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시'])
    .getDataRange().getValues();
  var list = [];
  var attended = 0, late = 0;
  for (var j = 1; j < stuRows.length; j++) {
    if (!stuRows[j][0]) continue;
    var uid = String(stuRows[j][0]).toUpperCase();
    var hit = done[uid] || null;
    if (hit) {
      attended++;
      if (hit.status === '지각') late++;
    }
    list.push({
      uid: uid,
      name: String(stuRows[j][1]),
      grade: String(stuRows[j][2]),
      klass: String(stuRows[j][3]),
      number: String(stuRows[j][4]),
      checked: !!hit,
      status: hit ? hit.status : '',
      time: hit ? hit.time : ''
    });
  }

  return json({
    ok: true, date: date, period: period,
    total: list.length, attended: attended, late: late,
    absent: list.length - attended,
    students: list
  });
}

// 출석/등록 세션 시작. 세션은 리더기(device) 1대당 1개, 캐시에 저장(자동 만료)
function startSession(p) {
  if (!p.device || (p.mode !== 'attend' && p.mode !== 'register')) {
    return json({ ok: false, error: 'device와 mode(attend|register)가 필요합니다' });
  }
  if (p.mode === 'register' && !p.name) {
    return json({ ok: false, error: '등록 모드는 name(학생 이름)이 필요합니다' });
  }
  var timeout = Math.min(Math.max(parseInt(p.timeout || '30', 10), 10), 300);
  // 등록은 학생 정보 입력 → 카드 꺼내기 → 태그까지 시간이 걸려서 30초로는 부족하다.
  // 앱이 짧은 값을 보내도 등록 세션은 최소 120초를 보장한다.
  // (앱은 응답의 timeout 값으로 카운트다운을 맞추면 된다)
  if (p.mode === 'register') timeout = Math.max(timeout, 120);

  // 지각 기준: 파라미터로 안 주면 교시설정 시트에서 찾는다
  var lateAfter = p.lateAfter || '';
  if (p.mode === 'attend' && !lateAfter && p.period) {
    lateAfter = findLateAfter(p.period);
  }

  var sess = {
    mode: p.mode,             // attend | register
    state: 'waiting',         // waiting → done
    device: p.device,
    name: p.name || '',       // 대상 학생 (출석 모드에서 비우면 아무나 태그 가능)
    grade: p.grade || '',
    klass: p.klass || '',
    number: p.number || '',
    period: p.period || '',   // 교시 (예: "1")
    lateAfter: lateAfter,     // 이 시각(HH:MM) 이후 태그는 지각
    result: null
  };
  CacheService.getScriptCache().put(cacheKey(p.device), JSON.stringify(sess), timeout);
  return json({ ok: true, timeout: timeout, lateAfter: lateAfter });
}

// 앱이 1초마다 물어보는 세션 상태
// state: none(세션 없음/만료) | waiting(태그 대기) | done(결과 있음)
function sessionStatus(p) {
  var sess = loadSession(p.device);
  if (!sess) return json({ state: 'none' });
  return json({ state: sess.state, mode: sess.mode, result: sess.result });
}

function cancelSession(p) {
  CacheService.getScriptCache().remove(cacheKey(p.device));
  return json({ ok: true });
}

// 등록된 학생 목록 (앱의 학생 선택 화면용)
function listStudents(p) {
  var sheet = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  var rows = sheet.getDataRange().getValues();
  var students = [];
  for (var i = 1; i < rows.length; i++) {
    if (!rows[i][0]) continue;
    students.push({
      uid: String(rows[i][0]),
      name: String(rows[i][1]),
      grade: String(rows[i][2]),
      klass: String(rows[i][3]),
      number: String(rows[i][4]),
      registeredAt: String(rows[i][5])
    });
  }
  return json({ ok: true, students: students });
}

// 출석기록 조회 (앱의 출결 관리 화면용). date 기본값은 오늘
function listRecords(p) {
  var sheet = getSheet(SHEET_LOG, ['날짜', '시각', '교시', 'UID', '이름', '상태', '리더기']);
  var date = p.date || now('yyyy-MM-dd');
  var rows = sheet.getDataRange().getValues();
  var records = [];
  for (var i = 1; i < rows.length; i++) {
    var rowDate = formatCellDate(rows[i][0]);
    if (rowDate !== date) continue;
    if (p.period && String(rows[i][2]) !== String(p.period)) continue;
    records.push({
      date: rowDate,
      time: formatCellTime(rows[i][1]),
      period: String(rows[i][2]),
      uid: String(rows[i][3]),
      name: String(rows[i][4]),
      status: String(rows[i][5]),
      device: String(rows[i][6])
    });
  }
  return json({ ok: true, date: date, records: records });
}

// 교시설정 목록
function listPeriods(p) {
  var sheet = getPeriodsSheet();
  var rows = sheet.getDataRange().getValues();
  var periods = [];
  for (var i = 1; i < rows.length; i++) {
    if (rows[i][0] === '') continue;
    periods.push({
      period: String(rows[i][0]),
      start: formatCellTime(rows[i][1]),
      lateAfter: formatCellTime(rows[i][2])
    });
  }
  return json({ ok: true, periods: periods });
}

// ────────────────────────── ESP32 쪽 ──────────────────────────

// ESP32가 2초마다 물어보는 "할 일" — 한 단어로만 답한다
function devicePoll(p) {
  var sess = loadSession(p.device);
  if (!sess || sess.state !== 'waiting') return text('IDLE');
  return text(sess.mode === 'register' ? 'REGISTER' : 'ATTEND');
}

// ESP32가 카드를 태그했을 때. 시트 기록까지 여기서 처리한다
function deviceTag(p) {
  if (!p.device || !p.uid) return text('ERR,param');
  var lock = LockService.getScriptLock();   // 동시 태그로 시트가 꼬이지 않게
  lock.waitLock(5000);
  try {
    var sess = loadSession(p.device);
    if (!sess || sess.state !== 'waiting') return text('IDLE');

    var uid = String(p.uid).toUpperCase();
    var out = (sess.mode === 'register') ? doRegister(sess, uid) : doAttend(sess, uid);

    // 결과를 세션에 남겨서 앱이 status 폴링으로 가져가게 한다 (60초 유지)
    sess.state = 'done';
    CacheService.getScriptCache().put(cacheKey(p.device), JSON.stringify(sess), 60);
    return text(out);
  } finally {
    lock.releaseLock();
  }
}

// 등록 모드: UID를 학생명단에 저장
function doRegister(sess, uid) {
  var sheet = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  var found = findStudent(uid);
  if (found) {
    sess.result = { ok: false, reason: 'dup', name: found.name };
    return 'DUP,' + found.name;
  }
  sheet.appendRow([uid, sess.name, sess.grade, sess.klass, sess.number, now('yyyy-MM-dd HH:mm:ss')]);
  sess.result = { ok: true, name: sess.name, uid: uid };
  return 'OK_REG,' + sess.name;
}

// 출석 모드: UID로 학생을 찾아 출석기록에 저장 (지각 판정·중복 방지 포함)
function doAttend(sess, uid) {
  var student = findStudent(uid);
  if (!student) {
    sess.result = { ok: false, reason: 'unknown', uid: uid };
    return 'UNKNOWN';
  }
  // 특정 학생을 대상으로 한 세션이면, 다른 학생 카드는 거절
  if (sess.name && sess.name !== student.name) {
    sess.result = { ok: false, reason: 'mismatch', name: student.name };
    return 'MISMATCH,' + student.name;
  }

  var sheet = getSheet(SHEET_LOG, ['날짜', '시각', '교시', 'UID', '이름', '상태', '리더기']);
  var today = now('yyyy-MM-dd');

  // 같은 날 같은 교시에 이미 출석한 학생이면 중복 기록하지 않는다
  var rows = sheet.getDataRange().getValues();
  for (var i = 1; i < rows.length; i++) {
    if (formatCellDate(rows[i][0]) === today &&
        String(rows[i][2]) === String(sess.period) &&
        String(rows[i][3]).toUpperCase() === uid) {
      var prevTime = formatCellTime(rows[i][1]);
      sess.result = { ok: false, reason: 'already', name: student.name, time: prevTime, status: String(rows[i][5]) };
      return 'ALREADY,' + student.name + ',' + prevTime;
    }
  }

  var time = now('HH:mm:ss');
  var status = '출석';
  if (sess.lateAfter && time > sess.lateAfter + ':00') status = '지각';

  sheet.appendRow([today, time, sess.period, uid, student.name, status, sess.device]);
  sess.result = { ok: true, name: student.name, status: status, time: time };
  return 'OK,' + student.name + ',' + status + ',' + time;
}

// ────────────────────────── 도우미 ──────────────────────────

function cacheKey(device) { return 'sess_' + device; }

function loadSession(device) {
  if (!device) return null;
  var raw = CacheService.getScriptCache().get(cacheKey(device));
  return raw ? JSON.parse(raw) : null;
}

// UID로 학생명단에서 학생 찾기
function findStudent(uid) {
  var sheet = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  var rows = sheet.getDataRange().getValues();
  for (var i = 1; i < rows.length; i++) {
    if (String(rows[i][0]).toUpperCase() === uid) {
      return { name: String(rows[i][1]), grade: rows[i][2], klass: rows[i][3], number: rows[i][4] };
    }
  }
  return null;
}

// 교시설정 시트에서 해당 교시의 지각기준(HH:MM) 찾기
function findLateAfter(period) {
  var sheet = getPeriodsSheet();
  var rows = sheet.getDataRange().getValues();
  for (var i = 1; i < rows.length; i++) {
    if (String(rows[i][0]) === String(period)) return formatCellTime(rows[i][2]).slice(0, 5);
  }
  return '';
}

// 어떤 요청이든 처음 들어올 때 세 시트를 한 번에 만들어 둔다.
// (이렇게 해야 배포 직후 action=periods 한 번만 열어도 시트 구조 전체를 볼 수 있다)
function ensureSheets() {
  getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  getSheet(SHEET_LOG, ['날짜', '시각', '교시', 'UID', '이름', '상태', '리더기']);
  getPeriodsSheet();
  getConfigSheet();
}

// 설정 시트: 없으면 기본값으로 만든다 (비밀번호는 시트에서 직접 바꿔 쓰세요)
function getConfigSheet() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(SHEET_CONFIG);
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_CONFIG);
    sheet.appendRow(['항목', '값']);
    sheet.setFrozenRows(1);
    sheet.appendRow(['교사비밀번호', '1234']);
    sheet.appendRow(['비밀번호보호', 'OFF']);   // ON으로 바꾸면 관리 기능에 pin 필요
    sheet.getRange('B2:B10').setNumberFormat('@');  // 0으로 시작하는 번호도 그대로
  }
  return sheet;
}

// 설정 시트에서 항목 값 읽기
function getConfig(key) {
  var rows = getConfigSheet().getDataRange().getValues();
  for (var i = 1; i < rows.length; i++) {
    if (String(rows[i][0]).trim() === key) return String(rows[i][1]).trim();
  }
  return '';
}

// 비밀번호 보호가 켜져 있는가
function isProtected() {
  return getConfig('비밀번호보호').toUpperCase() === 'ON';
}

// pin이 맞는가 (보호가 꺼져 있으면 언제나 통과)
function checkPin(pin) {
  if (!isProtected()) return true;
  var real = getConfig('교사비밀번호');
  if (!real) return true;               // 비밀번호가 비어 있으면 잠그지 않는다
  return String(pin || '') === real;
}

// 시트가 없으면 머리글과 함께 만들어서 돌려준다
function getSheet(name, headers) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(name);
  if (!sheet) {
    sheet = ss.insertSheet(name);
    sheet.appendRow(headers);
    sheet.setFrozenRows(1);
  }
  return sheet;
}

// 교시설정 시트: 없으면 기본 시간표를 채워서 만든다 (학교에 맞게 시트에서 직접 수정)
function getPeriodsSheet() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(SHEET_PERIODS);
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_PERIODS);
    sheet.appendRow(['교시', '시작시각', '지각기준']);
    sheet.setFrozenRows(1);
    var defaults = [
      ['1', '09:00', '09:10'],
      ['2', '10:00', '10:10'],
      ['3', '11:00', '11:10'],
      ['4', '12:00', '12:10'],
      ['5', '14:00', '14:10'],
      ['6', '15:00', '15:10'],
      ['7', '16:00', '16:10']
    ];
    for (var i = 0; i < defaults.length; i++) sheet.appendRow(defaults[i]);
    // 시각이 날짜로 자동 변환되지 않게 텍스트 서식 지정
    sheet.getRange('B2:C100').setNumberFormat('@');
  }
  return sheet;
}

function now(format) {
  return Utilities.formatDate(new Date(), 'Asia/Seoul', format);
}

// 시트 셀 값이 Date 객체로 읽히는 경우까지 처리해서 문자열로 통일
function formatCellDate(v) {
  if (v instanceof Date) return Utilities.formatDate(v, 'Asia/Seoul', 'yyyy-MM-dd');
  return String(v);
}

function formatCellTime(v) {
  if (v instanceof Date) return Utilities.formatDate(v, 'Asia/Seoul', 'HH:mm:ss');
  return String(v);
}

function text(s) {
  return ContentService.createTextOutput(s).setMimeType(ContentService.MimeType.TEXT);
}

function json(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(ContentService.MimeType.JSON);
}
