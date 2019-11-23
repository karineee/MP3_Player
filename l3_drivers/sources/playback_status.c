#include "FreeRTOS.h"
#include "queue_manager.h"
#include "semphr.h"
#include <stdbool.h>
#include <string.h>

song_status status;
SemaphoreHandle_t lock;

void playback_status_init(void) {
  status.inPlay = false;
  status.inPause = false;
  status.noSong = true;
  lock = xSemaphoreCreateMutex();
}

bool playback__is_paused(void) {
  bool paused_status;
  xSemaphoreTake(lock, portMAX_DELAY);
  paused_status = status.inPause;
  xSemaphoreGive(lock);
  return paused_status;
}
bool playback__is_playing(void) {
  bool play_status;
  xSemaphoreTake(lock, portMAX_DELAY);
  play_status = status.inPlay;
  xSemaphoreGive(lock);
  return play_status;
}
bool playback__no_song(void) {
  bool nosong_status;
  xSemaphoreTake(lock, portMAX_DELAY);
  nosong_status = status.noSong;
  xSemaphoreGive(lock);
  return nosong_status;
}

void playback__set_playing() {
  xSemaphoreTake(lock, portMAX_DELAY);
  status.inPlay = true;
  xSemaphoreGive(lock);
}
void playback__clear_playing() {
  xSemaphoreTake(lock, portMAX_DELAY);
  status.inPlay = false;
  xSemaphoreGive(lock);
}
void playback__set_pause() {
  xSemaphoreTake(lock, portMAX_DELAY);
  status.inPause = true;
  xSemaphoreGive(lock);
}
void playback__clear_pause() {
  xSemaphoreTake(lock, portMAX_DELAY);
  status.inPause = false;
  xSemaphoreGive(lock);
}