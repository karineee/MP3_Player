#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "queue.h"
#include "sj2_cli.h"
#include "task.h"

#include "ff.h"
#include "sl_string.h"

#include <stdio.h>

typedef struct File_contents {
  char songname[32];
} File_list;

File_list file;
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

void main(void) {
  Q_songname = xQueueCreate(sizeof(char[32]), 1);
  Q_songdata = xQueueCreate(512, 1);
}

char fpeek(FILE *stream) {
  char c;

  c = fgetc(stream);
  ungetc(c, stream);

  return c;
}

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  char name[32];
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songname, &name, portMAX_DELAY);
    printf("Received song to play: %s\n", name);

    FILE *mp3file; // File handle
    FRESULT result = f_open(&file, name, (FA_READ));

    char c = fpeek(mp3file);

    while (c != EOF) {
      read_from_file(bytes_512);
      xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
      char c = fpeek(mp3file);
    }
    close_file();
  }
}

void find_mp3_by_filename(char name[32]) {
  FIL file; // File handle
  FRESULT result = f_open(&file, name, (FA_READ));
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      while (!mp3_decoder_needs_data()) {
        vTaskDelay(1);
      }

      spi_send_to_mp3_decoder(bytes_512[i]);
    }
  }
}

void end() {}