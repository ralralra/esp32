/*
  4회차 · 따라하기 ② — 첫 서버 코드: doGet
  3회차의 server.on("/")과 완전히 같은 역할 — 자리만 ESP32에서 구글로!

  사용법: 구글 시트 → 확장 프로그램 → Apps Script → 붙여넣기 → 저장
         → [배포] → [새 배포] → 유형 '웹 앱' → 액세스 '모든 사용자' → 배포
  테스트: 웹앱URL?temp=25&humi=48&light=800&sound=120 을 브라우저로 열기
         → 화면에 OK + 시트에 새 줄이 생기면 서버 개통 성공!
*/

function doGet(e) {
  // e.parameter = 주소 뒤 ?이름=값 묶음
  const sheet = SpreadsheetApp
    .getActiveSpreadsheet()
    .getSheetByName("시트1");   // ★ 시트 탭 이름과 똑같아야!

  const t = e.parameter.temp;   // 온도
  const h = e.parameter.humi;   // 습도
  const l = e.parameter.light;  // 조도
  const s = e.parameter.sound;  // 소음

  sheet.appendRow([new Date(), t, h, l, s]);
  // 시트 맨 아래에 한 줄 추가!

  return ContentService.createTextOutput("OK");
}
