# V1 중계 서버

V1·V2와 12팀이 **같은 Apps Script 하나**를 씁니다 → **[`../../apps_script/README.md`](../../apps_script/README.md)** (만들기 · 주소 약속)

V1(전구)이 쓰는 것만 추리면:

| 구분 | 내용 |
|---|---|
| 명령 | `ON` · `OFF` · `AUTO` · `CONFIG` (`COLOR`·`BRIGHT`는 받아도 무시) |
| 보고 | `type=bulb` · `power` · `auto` · `light`(밝기 %) · `motion` · `dark` · `off` |
| 펌웨어 | [`../firmware/step4_app_lamp/step4_app_lamp.ino`](../firmware/step4_app_lamp/step4_app_lamp.ino) |
