#include "mp3_driver.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "semphr.h"
#include "ssp_lab.h"
#include <stdbool.h>
#include <string.h>

#define MAX_VOLUME 0x5F

song_status status;
SemaphoreHandle_t status_lock;

void playback_status_init(void) {
  status.inPlay = false;
  status.inPause = false;
  status.noSong = true;
  status_lock = xSemaphoreCreateMutex();
}

bool playback__is_paused(void) {
  bool paused_status;
  xSemaphoreTake(status_lock, portMAX_DELAY);
  paused_status = status.inPause;
  xSemaphoreGive(status_lock);
  return paused_status;
}
bool playback__is_playing(void) {
  bool play_status;
  xSemaphoreTake(status_lock, portMAX_DELAY);
  play_status = status.inPlay;
  xSemaphoreGive(status_lock);
  return play_status;
}
bool playback__no_song(void) {
  bool nosong_status;
  xSemaphoreTake(status_lock, portMAX_DELAY);
  nosong_status = status.noSong;
  xSemaphoreGive(status_lock);
  return nosong_status;
}

void playback__set_playing() {
  xSemaphoreTake(status_lock, portMAX_DELAY);
  status.inPlay = true;
  xSemaphoreGive(status_lock);
}
void playback__clear_playing() {
  xSemaphoreTake(status_lock, portMAX_DELAY);
  status.inPlay = false;
  xSemaphoreGive(status_lock);
}
void playback__set_pause() {
  xSemaphoreTake(status_lock, portMAX_DELAY);
  status.inPause = true;
  xSemaphoreGive(status_lock);
}
void playback__clear_pause() {
  xSemaphoreTake(status_lock, portMAX_DELAY);
  status.inPause = false;
  xSemaphoreGive(status_lock);
}

void playback__toggle_pause() {
  xQueueSemaphoreTake(status_lock, portMAX_DELAY);
  if (status.inPause) {
    status.inPause = false;
  } else {
    status.inPause = true;
  }

  xSemaphoreGive(status_lock);
}

gpio_s data_select, control_select, data_request;

void decoder__init(uint16_t reset_volume) {
  data_select = gpio__construct_as_output(GPIO__PORT_2, 7);
  control_select = gpio__construct_as_output(GPIO__PORT_2, 8);
  data_request = gpio__construct_as_input(GPIO__PORT_2, 6);
  gpio_s reset = gpio__construct_as_output(GPIO__PORT_2, 9);

  gpio__set(data_select);
  gpio__set(control_select);

  gpio__set(reset);
  gpio__reset(reset);
  gpio__set(reset);

  while (!gpio__get(data_request)) {
  }

  decoder__write_reg(0x0B, reset_volume);
  decoder__write_reg(0x03, 0x6000);
}
void decoder__write_reg(uint8_t register_address, uint16_t data) {
  gpio__reset(control_select);

  ssp0__exchange_byte(0x02);
  ssp0__exchange_byte(register_address);

  ssp0__exchange_half(data);

  gpio__set(control_select);
  while (!gpio__get(data_request)) {
  }
}
void decoder__write_data(uint8_t data) {
  gpio__reset(data_select);
  ssp0__exchange_byte(data);
  gpio__set(data_select);
}
