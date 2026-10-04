// 시뮬레이터용 FreeRTOS 흉내 — 큐만 진짜로 동작하고, 태스크는 만들지 않습니다.
#pragma once
#include <cstring>
#include <deque>
#include <map>
#include <vector>
#include <cstddef>

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS  1
#define pdMS_TO_TICKS(x) (x)
typedef unsigned long TickType_t;

struct SimQueue { size_t itemSize; size_t capacity; std::deque<std::vector<char>> items; };
typedef SimQueue* QueueHandle_t;

inline QueueHandle_t xQueueCreate(size_t len, size_t itemSize) {
  return new SimQueue{itemSize, len, {}};
}
inline int xQueueSend(QueueHandle_t q, const void* item, TickType_t) {
  if (!q || q->items.size() >= q->capacity) return pdFALSE;
  std::vector<char> buf(q->itemSize);
  std::memcpy(buf.data(), item, q->itemSize);
  q->items.push_back(buf);
  return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q, void* out, TickType_t) {
  if (!q || q->items.empty()) return pdFALSE;
  std::memcpy(out, q->items.front().data(), q->itemSize);
  q->items.pop_front();
  return pdTRUE;
}
inline size_t uxQueueSpacesAvailable(QueueHandle_t q) {
  return q ? q->capacity - q->items.size() : 0;
}
inline void vTaskDelay(TickType_t) {}
inline int xTaskCreatePinnedToCore(void (*)(void*), const char*, int, void*, int, void*, int) {
  return pdPASS;                 // 시뮬레이터에서는 통신 태스크를 돌리지 않습니다
}
