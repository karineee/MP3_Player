#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_int.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdio.h>

#include "queue_manager.h"
QueueHandle_t Song_Q, Data_Q;

void read_song_name_task(void *params);
void send_data_to_decode(void *params);

int main(void) {
  Song_Q = xQueueCreate(1, sizeof(name));
  Data_Q = xQueueCreate(3, sizeof(char[1024]));

  sj2_cli__init();

  xTaskCreate(read_song_name_task, "Read_Song", 2048, NULL, 2, NULL);
  xTaskCreate(send_data_to_decode, "Send_Song", 2048, NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 1;
}

void read_song_name_task(void *params) {
  char name[32];

  FIL mp3file;
  UINT bytes_read, total_read;
  FRESULT result;
  char byte;

  UINT file_size;

  char bytes_to_send[1024];

  while (1) {

    xQueueReceive(Song_Q, &name, portMAX_DELAY);
    printf("Playing Song: %s\n", name);

    result = f_open(&mp3file, name, FA_READ | FA_OPEN_EXISTING);

    if (result == FR_OK) { // file does exist!
      file_size = f_size(&mp3file);
      total_read = 0;

      while (file_size > total_read) {
        f_read(&mp3file, &bytes_to_send, 1024, &bytes_read);
        total_read += bytes_read;
        xQueueSend(Data_Q, bytes_to_send, portMAX_DELAY);
      }

      f_close(&mp3file);
    }
    vTaskDelay(1000);
  }
}

void mp3_decoder_hardware_reset() {
  gpio_s reset = gpio__construct_as_output(GPIO__PORT_2, 6);
  gpio__set(reset);
  gpio__reset(reset);
  vTaskDelay(100);
  gpio__set(reset);
}

void send_data_to_decode(void *params) {

  char bytes[1024];
  ssp0__init(24);

  gpio_s data_select, control_select, data_request;
  data_select = gpio__construct_as_output(GPIO__PORT_2, 7);
  control_select = gpio__construct_as_output(GPIO__PORT_2, 8);
  data_request = gpio__construct_as_input(GPIO__PORT_2, 9);

  mp3_decoder_hardware_reset();
  while (!gpio__get(data_request)) {
    vTaskDelay(1);
  }

  while (1) {

    xQueueReceive(Data_Q, bytes, portMAX_DELAY);

    gpio__reset(data_select);
    gpio__set(control_select);
    while (!gpio__get(data_request)) {
      vTaskDelay(1);
    }
    for (int i = 0; i < 1024; ++i) {
      ssp0__exchange_byte(bytes[i]);
      if ((i % 32) == 0) {
        while (!gpio__get(data_request)) {
          vTaskDelay(1);
        }
      }
    }
  }
}

void nothing_task(void *params) {

  while (1) {

    printf("Doing Nothing\n");

    vTaskDelay(5000);
  }
}
