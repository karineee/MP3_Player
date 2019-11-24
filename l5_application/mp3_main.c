#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>

#include "gpio_int.h"

#include "char_map.h"
#include "queue_manager.h"
QueueHandle_t Song_Q, Data_Q;

SemaphoreHandle_t Pause_Signal;

void read_song_name_task(void *params);
void send_data_to_decode(void *params);
void read_pause_task(void *params);

void send_pause_signal_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 0);

  xSemaphoreGiveFromISR(Pause_Signal, NULL);
};

int main(void) {
  Song_Q = xQueueCreate(1, sizeof(name));
  Data_Q = xQueueCreate(3, sizeof(char[512]));

  playback_status_init();
  sj2_cli__init();

  Pause_Signal = xSemaphoreCreateBinary();
  gpio_s pause_button = gpio__construct_as_input(GPIO__PORT_2, 0);

  gpio2__attach_interrupt(0, GPIO_INTR__RISING_EDGE, send_pause_signal_isr);

  xTaskCreate(read_song_name_task, "Read_Song", 2048, NULL, 2, NULL);
  xTaskCreate(send_data_to_decode, "Send_Song", 2048, NULL, 2, NULL);
  xTaskCreate(read_pause_task, "Pause Task", 1024, NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 1;
}

void read_song_name_task(void *params) {
  char name[32];
  FIL mp3file;
  UINT bytes_read, total_read;
  FRESULT result;

  UINT file_size;

  char bytes_to_send[512];

  while (1) {

    xQueueReceive(Song_Q, &name, portMAX_DELAY);
    printf("Playing Song: %s\n", name);
    playback__set_playing();
    result = f_open(&mp3file, name, FA_READ | FA_OPEN_EXISTING);

    if (result == FR_OK) { // file does exist!
      file_size = f_size(&mp3file);
      total_read = 0;

      while (file_size > total_read) {
        f_read(&mp3file, &bytes_to_send, 512, &bytes_read);
        total_read += bytes_read;

        while (playback__is_paused()) {
          vTaskDelay(1);
        }

        xQueueSend(Data_Q, bytes_to_send, portMAX_DELAY);
      }

      f_close(&mp3file);
      playback__clear_playing();
    }
    vTaskDelay(1000);
  }
}

void mp3_decoder_hardware_reset() {
  gpio_s reset = gpio__construct_as_output(GPIO__PORT_2, 9);
  gpio__set(reset);
  gpio__reset(reset);
  vTaskDelay(100);
  gpio__set(reset);
}

void send_data_to_decode(void *params) {
  char bytes[512];
  ssp0__init(24);

  gpio_s data_select, control_select, data_request;
  data_select = gpio__construct_as_output(GPIO__PORT_2, 7);
  control_select = gpio__construct_as_output(GPIO__PORT_2, 8);
  data_request = gpio__construct_as_input(GPIO__PORT_2, 6);

  mp3_decoder_hardware_reset();
  gpio__reset(data_select);
  gpio__set(control_select);
  while (!gpio__get(data_request)) {
    vTaskDelay(1);
  }

  // volume changing
  gpio__set(data_select);
  gpio__reset(control_select);

  ssp0__exchange_byte(0x02);
  ssp0__exchange_byte(0x0B);

  ssp0__exchange_half(0x7070);

  gpio__set(control_select);
  // volume changing
  while (!gpio__get(data_request)) {
    vTaskDelay(1);
  }

  int index;
  while (1) {

    xQueueReceive(Data_Q, bytes, portMAX_DELAY);

    gpio__reset(data_select);
    gpio__set(control_select);

    while (!gpio__get(data_request)) {
    }

    for (int i = 0; i < 16; ++i) {
      for (int j = 0; j < 32; ++j) {
        index = i * 32 + j;
        ssp0__exchange_byte(bytes[index]);
      }

      while (!gpio__get(data_request)) {
      }
    }
    vTaskDelay(1);
  }
}

void read_pause_task(void *params) {

  while (1) {
    xSemaphoreTake(Pause_Signal, portMAX_DELAY);
    playback__toggle_pause();
  }
}


void volume_control_task(void *params) {
  uint16_t volume; 
  
  while (1) {
    xSemaphoreTake(Pause_Signal, portMAX_DELAY);
    playback__toggle_pause();
  }
}