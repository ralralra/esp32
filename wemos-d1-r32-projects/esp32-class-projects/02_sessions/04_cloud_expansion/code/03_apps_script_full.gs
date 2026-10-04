/*
  4회차 · 배포 템플릿 — Apps Script 전체 버전 (팀 프로젝트의 뼈대!)
  mode 파라미터로 '요청 종류'를 구분해요:
    (없음)       → 센서 데이터 기록      : URL?temp=25&humi=48&light=800&sound=120
    mode=cmd    → 명령 셀(A1) 값 응답   : ESP32가 10초마다 확인 (폴링)
    mode=set    → 명령 셀에 값 쓰기      : URL?mode=set&cmd=ON (웹앱 버튼용)
    mode=latest → 최근 1줄 JSON 응답    : 웹앱 화면 표시용

  준비물: 시트 탭 2개 — '시트1'(데이터), '설정'(A1 = 명령 셀)
  ⚠ 수정하면 반드시 재배포! (배포 관리 → 연필 → 새 버전)
*/

function doGet(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const mode = e.parameter.mode;

  // ── 명령 읽기 (ESP32의 폴링) ──────────────────────────
  if (mode == "cmd") {
    const s = ss.getSheetByName("설정");
    return ContentService.createTextOutput(s.getRange("A1").getValue());
  }

  // ── 명령 쓰기 (웹앱 버튼) ─────────────────────────────
  if (mode == "set") {
    const s = ss.getSheetByName("설정");
    s.getRange("A1").setValue(e.parameter.cmd); // ON / OFF (대소문자 주의!)
    return ContentService.createTextOutput("OK");
  }

  // ── 최근 1줄 응답 (웹앱 화면 표시) ─────────────────────
  if (mode == "latest") {
    const sheet = ss.getSheetByName("시트1");
    const last = sheet.getLastRow();
    if (last < 2) return ContentService.createTextOutput("{}");
    const row = sheet.getRange(last, 1, 1, 5).getValues()[0];
    const json = {
      time: row[0], temp: row[1], humi: row[2],
      light: row[3], sound: row[4]
    };
    return ContentService.createTextOutput(JSON.stringify(json))
      .setMimeType(ContentService.MimeType.JSON);
  }

  // ── 기본: 센서 데이터 기록 ────────────────────────────
  const sheet = ss.getSheetByName("시트1");
  sheet.appendRow([
    new Date(),
    e.parameter.temp,
    e.parameter.humi,
    e.parameter.light,
    e.parameter.sound
  ]);
  return ContentService.createTextOutput("OK");
}
