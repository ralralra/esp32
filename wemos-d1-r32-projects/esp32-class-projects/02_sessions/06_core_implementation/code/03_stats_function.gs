/*
  6회차 · 3교시 — Apps Script 집계: 인기 순위 만들기
  판매로그를 세면 인기 순위가 돼요 — '엑셀 COUNTIF의 코드 버전'

  doGet의 분기에 연결: if (a == "stats") return stats();
  응용: 화분팀 = 최근 24시간 평균, 우산팀 = 이용 시간 계산
*/

function stats() {
  const log = SpreadsheetApp.getActiveSpreadsheet()
    .getSheetByName("판매로그");

  const rows = log.getDataRange().getValues();  // 시트 전체를 표(2차원 배열)로

  const count = {};
  for (let i = 1; i < rows.length; i++) {  // ★ i=1부터! 0번째 줄은 '제목 행'
    const item = rows[i][1];               // 두 번째 열 = 상품명
    count[item] = (count[item] || 0) + 1;
  }

  // 응답 예: {"cola":12, "chip":7, ...} → 웹앱이 받아 순위 표시
  return ContentService.createTextOutput(JSON.stringify(count))
    .setMimeType(ContentService.MimeType.JSON);
}
