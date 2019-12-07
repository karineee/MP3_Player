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

#include "mp3_driver.h"

#include "char_map.h"
#include "common_macros.h"
#include "gpio_int.h"
#include "ssp2_lab.h"
#include <stdbool.h>

#define MAX_VOLUME 0x5F
#define reset_volume 0x7070
#define data_size 1024

QueueHandle_t Song_Q, Data_Q, button2_Q;
QueueHandle_t volume_direction, scroll_direction;

SemaphoreHandle_t Pause_Signal;
SemaphoreHandle_t Button2_Signal;
SemaphoreHandle_t Decoder_lock;

gpio_s data_request;
bool change_song;
void read_song_name_task(void *params);
void send_data_to_decode(void *params);
void read_pause_task(void *params);
void volume_control_task(void *params);
void lcd_task(void *params);
void read_button2_task(void *params);

void send_pause_signal_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 0);
  xSemaphoreGiveFromISR(Pause_Signal, NULL);
};

void send_button2_signal_isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 6);
  xSemaphoreGiveFromISR(Button2_Signal, NULL);
};

void volume_change_isr(void) {
  LPC_GPIOINT->IO2IntClr |= (1 << 1);
  bool A = LPC_GPIO2->PIN & (1 << 1);
  bool B = LPC_GPIO2->PIN & (1 << 2);
  bool direction = A != B;
  xQueueSendToBackFromISR(volume_direction, &direction, NULL);
}

void scroll_change_isr(void) {
  LPC_GPIOINT->IO0IntClr |= (1 << 7);
  bool E = LPC_GPIO0->PIN & (1 << 7);
  bool F = LPC_GPIO0->PIN & (1 << 8);
  bool direction = E != F;
  xQueueSendToBackFromISR(scroll_direction, &direction, NULL);
}

int main(void) {
  Song_Q = xQueueCreate(1, sizeof(name));
  Data_Q = xQueueCreate(3, sizeof(char[data_size]));
  volume_direction = xQueueCreate(20, sizeof(bool));
  scroll_direction = xQueueCreate(10, sizeof(bool));
  button2_Q = xQueueCreate(1, sizeof(uint8_t));
  change_song = false;

  playback_status_init();
  sj2_cli__init();

  Pause_Signal = xSemaphoreCreateBinary();
  Button2_Signal = xSemaphoreCreateBinary();
  Decoder_lock = xSemaphoreCreateMutex();

  gpio_s pause_button = gpio__construct_as_input(GPIO__PORT_2, 0); // pin # to be same as attach interrupt and I02IntClr
  data_request = gpio__construct_as_input(GPIO__PORT_2, 6);
  gpio_s button2 = gpio__construct_as_input(GPIO__PORT_0, 6);

  gpio_s E, F;
  E = gpio__construct_as_input(GPIO__PORT_0, 7); // for next button  = third pin
  F = gpio__construct_as_input(GPIO__PORT_0, 8); // for next button = middle pin

  /*Never Used
  gpio_s C, D;
  C = gpio__construct_as_input(GPIO__PORT_1, 30); // for prev button = third pin
  D = gpio__construct_as_input(GPIO__PORT_1, 20); // for prev button = middle pin
  */

  gpio_s A, B;
  A = gpio__construct_as_input(GPIO__PORT_2, 1); // for pause button = third pin
  B = gpio__construct_as_input(GPIO__PORT_2, 2); // for pause button = middle pin

  gpio2__attach_interrupt(1, GPIO_INTR__RISING_EDGE, volume_change_isr);
  gpio2__attach_interrupt(1, GPIO_INTR__FALLING_EDGE, volume_change_isr);

  /* Uncomment when ready
  gpio0__attach_interrupt(7, GPIO_INTR__RISING_EDGE, scroll_change_isr);
  gpio0__attach_interrupt(7, GPIO_INTR__FALLING_EDGE, scroll_change_isr);
  */

  gpio2__attach_interrupt(0, GPIO_INTR__RISING_EDGE, send_pause_signal_isr);
  gpio0__attach_interrupt(6, GPIO_INTR__RISING_EDGE, send_button2_signal_isr);

  xTaskCreate(read_song_name_task, "Read_Song", data_size + 512, NULL, 2, NULL);
  xTaskCreate(send_data_to_decode, "Send_Song", data_size + 512, NULL, 2, NULL);
  xTaskCreate(read_pause_task, "Pause Task", 512, NULL, 3, NULL);
  xTaskCreate(volume_control_task, "Volume Task", 512, NULL, 3, NULL);

  // xTaskCreate(lcd_task, "Lcd Task", 2048, NULL, 2, NULL);
  xTaskCreate(read_button2_task, "Button 2 Task", 512, NULL, 4, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 1;
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

  char bytes_to_send[data_size];

  while (1) {

    xQueueReceive(Song_Q, &name, portMAX_DELAY);
    printf("Playing Song: %s\n", name);
    playback__set_playing();
    result = f_open(&mp3file, name, FA_READ | FA_OPEN_EXISTING);

    if (result == FR_OK) { // file does exist!
      file_size = f_size(&mp3file);
      total_read = 0;

      while (file_size > total_read) {
        while (playback__is_paused()) {
          vTaskDelay(1);
        }

        // take mutex
        if (change_song) {
          change_song = false;
          break;
        }
        // give mutex
        f_read(&mp3file, &bytes_to_send, data_size, &bytes_read);
        total_read += bytes_read;

        xQueueSend(Data_Q, bytes_to_send, portMAX_DELAY);
      }

      f_close(&mp3file);
      playback__clear_playing();
    }
    vTaskDelay(1000);
  }
}

void send_data_to_decode(void *params) {

  char bytes[data_size];

  ssp0__init(1);
  decoder__init(reset_volume);
  ssp0__init(4);

  int index;
  while (1) {
    while (playback__is_paused()) {
      vTaskDelay(1);
    }
    xQueueReceive(Data_Q, bytes, portMAX_DELAY);

    for (int i = 0; i < data_size / 32; ++i) {
      while (!gpio__get(data_request)) {
        vTaskDelay(1);
      }
      for (int j = 0; j < 32; ++j) {
        index = i * 32 + j;
        xSemaphoreTake(Decoder_lock, portMAX_DELAY);
        decoder__write_data(bytes[index]);
        xSemaphoreGive(Decoder_lock);
      }
    }
  }
}

void read_pause_task(void *params) {

  while (1) {
    xSemaphoreTake(Pause_Signal, portMAX_DELAY);
    playback__toggle_pause();
  }
}

void read_button2_task(void *params) {
  uint8_t state = 0;
  while (1) {
    state = 0;
    if (xSemaphoreTake(Button2_Signal, portMAX_DELAY)) {
      state = 1;
      if (xSemaphoreTake(Button2_Signal, 400)) {
        state = 2;
        if (xSemaphoreTake(Button2_Signal, 400)) {
          state = 3;
        }
      }
    }
    xQueueSend(button2_Q, &state, portMAX_DELAY);
  }
}

void proccess_button2(void) {
  uint8_t state;

  while (1) {
    xQueueReceive(button2_Q, &state, portMAX_DELAY);
    if (state == 1) { // 1 button press, play selected song.

      /*logic to get selected song name

      */

      // then
      // xQueueSend(Song_Q, &name, portMAX_DELAY);

    } else if (state == 2) { // 2 button press, play next song
      /*logic to get next song name

     */

      // then
      // xQueueSend(Song_Q, &name, portMAX_DELAY);

    } else if (state = 3) { // 3 button press, play previous song
      /*logic to get previous song name

     */

      // then
      // xQueueSend(Song_Q, &name, portMAX_DELAY);

    } else {
      // if received state makes it into here, do nothing
      // invalid state
    }
    // take mutex
    change_song = true;
    // give mutex
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
    decoder__write_reg(0x0B, volume);
    xSemaphoreGive(Decoder_lock);
  }
}
