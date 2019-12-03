#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"

#include "queue.h"
#include "sj2_cli.h"
#include "sl_string.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"

#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "queue_manager.h"
#include "sj2_cli.h"
#include "task.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "char_map.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_int.h"
#include "queue.h"
#include "queue_manager.h"
#include "sj2_cli.h"
#include "ssp2_lab.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>

#define MAX_VOLUME 0x5F
QueueHandle_t Song_Q, Data_Q;
QueueHandle_t volume_direction;

SemaphoreHandle_t Pause_Signal;
SemaphoreHandle_t Next_Signal;
SemaphoreHandle_t Prev_Signal;
SemaphoreHandle_t Decoder_lock;

gpio_s data_select, control_select, data_request;
uint16_t reset_volume;

void read_song_name_task(void *params);
void send_data_to_decode(void *params);
void read_pause_task(void *params);
void volume_control_task(void *params);
void lcd_task(void *params);
void read_prev_next_task(void *params);

void send_pause_signal_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 0);
  xSemaphoreGiveFromISR(Pause_Signal, NULL);
};
void send_prev_signal_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 31);
  xSemaphoreGiveFromISR(Prev_Signal, NULL);
};
void send_next_signal_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 8);
  xSemaphoreGiveFromISR(Next_Signal, NULL);
};

void volume_change_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 1);
  bool A = LPC_GPIO2->PIN & (1 << 1);
  bool B = LPC_GPIO2->PIN & (1 << 2);
  bool direction = A != B;
  xQueueSendToBackFromISR(volume_direction, &direction, NULL);
}

int main(void) {
  Song_Q = xQueueCreate(1, sizeof(name));
  Data_Q = xQueueCreate(3, sizeof(char[512]));
  volume_direction = xQueueCreate(20, sizeof(bool));

  reset_volume = 0x7070;

  playback_status_init();
  sj2_cli__init();

  Pause_Signal = xSemaphoreCreateBinary();
  Next_Signal = xSemaphoreCreateBinary();
  Prev_Signal = xSemaphoreCreateBinary();
  Decoder_lock = xSemaphoreCreateMutex();

  gpio_s pause_button = gpio__construct_as_input(GPIO__PORT_2, 0); // pin # to be same as attach interrupt and I02IntClr
  gpio_s prev_button = gpio__construct_as_input(GPIO__PORT_1, 31);
  gpio_s next_button = gpio__construct_as_input(GPIO__PORT_0, 8);
  data_select = gpio__construct_as_output(GPIO__PORT_2, 7);
  control_select = gpio__construct_as_output(GPIO__PORT_2, 8);
  data_request = gpio__construct_as_input(GPIO__PORT_2, 6);


  gpio_s E, F;
  E = gpio__construct_as_input(GPIO__PORT_0, 7); //for next button  = third pin
  F = gpio__construct_as_input(GPIO__PORT_0, 9); //for next button = middle pin

  gpio_s C, D;
  C = gpio__construct_as_input(GPIO__PORT_1, 30); //for prev button = third pin
  D = gpio__construct_as_input(GPIO__PORT_1, 20); //for prev button = middle pin

  gpio_s A, B;
  A = gpio__construct_as_input(GPIO__PORT_2, 1); // for pause button = third pin
  B = gpio__construct_as_input(GPIO__PORT_2, 2); // for pause button = middle pin


  gpio2__attach_interrupt(1, GPIO_INTR__RISING_EDGE, volume_change_isr);
  gpio2__attach_interrupt(1, GPIO_INTR__FALLING_EDGE, volume_change_isr);

  gpio2__attach_interrupt(0, GPIO_INTR__RISING_EDGE, send_pause_signal_isr);
  gpio2__attach_interrupt(31, GPIO_INTR__RISING_EDGE, send_prev_signal_isr);
  gpio2__attach_interrupt(8, GPIO_INTR__RISING_EDGE, send_next_signal_isr);

  xTaskCreate(read_song_name_task, "Read_Song", 2048, NULL, 2, NULL);
  xTaskCreate(send_data_to_decode, "Send_Song", 2048, NULL, 2, NULL);
  xTaskCreate(read_pause_task, "Pause Task", 1024, NULL, 2, NULL);
  xTaskCreate(volume_control_task, "Volume Task", 1024, NULL, 2, NULL);

  // xTaskCreate(lcd_task, "Lcd Task", 2048, NULL, 2, NULL);
  // xTaskCreate(read_prev_next_task, "Previous and Next Task", 2048, NULL, 2, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 1;
}

void read_prev_next_task(void *params) {
  bool next_signal = false;

  // if signal received then send it to the queue
  if (next_signal) {

    // send next song to the queue

    //  xQueueReceive(Song_Q, &name, portMAX_DELAY);
  }


}

void lcd_task(void *params) {

  // This opens a txt file and prints the values
  // char name[32];
  FIL mp3file;
  UINT bytes_read, total_read;

  FRESULT result;
  char byte;
  UINT file_size;
  char bytes_received[1024];
  const char EOL = '\n';
  int i;

  result = f_open(&mp3file, "Tracklist.txt", FA_READ | FA_OPEN_EXISTING);
  while (1) {
    if (result == FR_OK) { // file does exist!
      file_size = f_size(&mp3file);
      total_read = 0;

      /* while (file_size > total_read) {
         // reading to bytes_to_send
         f_read(&mp3file, &bytes_received, 1024, &bytes_read);
       } */

      for (int i = 0; i < file_size; i++) {
        f_read(&mp3file, &bytes_received, 1024, &bytes_read);
        total_read += bytes_read;
        fprintf(stderr, "%c", bytes_received[i]);
      }

      f_close(&mp3file);
    }
  }
  vTaskDelay(1000);
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
          vTaskDelay(1); // originally was 1
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

  ssp0__exchange_half(reset_volume);

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
        xSemaphoreTake(Decoder_lock, portMAX_DELAY);
        ssp0__exchange_byte(bytes[index]);
        xSemaphoreGive(Decoder_lock);
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
void read_next_task(void *params) {

  while (1) {
    xSemaphoreTake(Next_Signal, portMAX_DELAY);
    playback__toggle_next();
  }
}
void read_prev_task(void *params) {

  while (1) {
    xSemaphoreTake(Prev_Signal, portMAX_DELAY);
    playback__toggle_prev();
  }
}

void volume_control_task(void *params) {
  uint16_t volume;
  bool CnotCounterC;
  uint8_t channel_volume = reset_volume;

  while (1) {
    
    xQueueReceive(volume_direction, &CnotCounterC, portMAX_DELAY);

    if (CnotCounterC) {
      if (channel_volume > MAX_VOLUME) {
        channel_volume -= 0x01;
      }
    } else {
      if (channel_volume < 0xFE) {
        channel_volume += 0x01;
      }
    }
    volume = (channel_volume << 8) | (channel_volume << 0);

    xSemaphoreTake(Decoder_lock, portMAX_DELAY);
    gpio__set(data_select);
    gpio__reset(control_select);

    ssp0__exchange_byte(0x02);
    ssp0__exchange_byte(0x0B);

    ssp0__exchange_half(volume);

    gpio__set(control_select);
    gpio__reset(data_select);

    while (!gpio__get(data_request)) {
    }
    xSemaphoreGive(Decoder_lock);
  }
}