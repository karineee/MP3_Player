#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "gpio.h"
#include "mp3_interface.h"

#include "queue.h"
#include "sj2_cli.h"
#include "sl_string.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct File_contents {
  char songname[32];
} File_list;

File_list file;
QueueHandle_t SongQueue;
QueueHandle_t Q_songdata;

char fpeek(FILE *stream) {
  char c;

  c = fgetc(stream);
  ungetc(c, stream);

  return c;
}

void empty_task(void *p) {
  while (1) {
    fprintf(stderr, "Waiting in the empty task\n");
    vTaskDelay(20000);
  }
}

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  char name[32];
  char bytes_512[512];
  char bytes_512_2[512];

  fprintf(stderr, "Inside the mp3_reader_task \n");

  while (1) {

    // TODO: IMPLEMENT THESE TWO LINES
    xQueueReceive(SongQueue, &name, portMAX_DELAY);
    fprintf(stderr, "Received song to play: %s\n", name);

    const char *testName = "test.mp3";
    fprintf(stderr, "Testing: %s\n", name);
    /*
    FILE mp3File; // File handle
    FRESULT result = f_open(&mp3File, name, (FA_READ));
    */

    FILE *mp3File;
    if (NULL == mp3File = fopen(name, "r")) {
      // file of name does not exist
    }

    else {
      fprintf(stderr, "File Found: %s\n", name);
      fprintf(stderr, "Begin Queueing File Data!\n\n");

      // char c = fpeek(&mp3File);
      // printf("Just peeked: %c\n");

      int iteration = 0;
      char buffer[512];
      // while (c != EOF) {
      while (1) { // should be while(not eof)

        //**********************
        // This block should be in its own function
        // But f_read loses its place in the file or something
        // And the program crashes
        //**********************

        char c = "";
        UINT bytes_read = 1;
        for (int i = 0; i < 512; i++) { // should be 512

          f_read(&mp3File, &c, 1, &bytes_read);
          buffer[i] = c;
          fprintf(stderr, "r - # %d: %c, in integer form: %d\n", i + (iteration * 512), c, c);
          fprintf(stderr, "Reader: %c", buffer[i]);
        }

        //**********************

        fprintf(stderr, "\nBlock #%d read\n", iteration);
        vTaskDelay(2000);
        // read_from_file(bytes_512_2, mp3File);
        // printf("Just finished reading bytes 2\n");
        // vTaskDelay(5000);
        xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
        printf("Just sent 512 bytes to Q_songdata!\n");
        // char c = fpeek(mp3File);
        vTaskDelay(2000);
        iteration++;
      }

      f_close(&mp3File);
      printf("We read the file and reached the end\n");
      vTaskDelay(5000);
    }
    else {
      printf("ERROR: Failed to open: %s\n", name);
      vTaskDelay(10000);
    }
  }
}

void read_from_file(char bytes_512[512], FILE mp3File) {
  char buffer[512];
  char c = "";
  UINT bytes_read = 1;

  for (int i = 0; i < 512; i++) { // should be 512

    f_read(&mp3File, &c, 1, &bytes_read);
    buffer[i] = c;
    printf("# %d: %c. in integer form: %d\n", i, c, c);
  }

  printf("read_from_file: Just placed 512 bytes into buffer\n");
}

void find_mp3_by_filename(char name[32]) {
  FIL file; // File handle
  FRESULT result = f_open(&file, name, (FA_READ));
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {

  fprintf(stderr, "Starting mp3_player_task");
  char bytes_512[512];
  spi0_init();
  while (1) {

    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < 512; i++) {
      while (!get_data_request_signal()) {
        fprintf(stderr, "while\n");
        vTaskDelay(1);
      }
      // Set Data CS before sending bytes
      printf("p - #%d: %c\n", i, bytes_512[i]);
      sp0_ds();
      ssp0_exchange_byte(bytes_512[i]);
      // fprintf(stderr, "Exchanging bytes");
    }
  }
}

void main(void) {
  SongQueue = xQueueCreate(1, sizeof(char[32]));
  Q_songdata = xQueueCreate(1, sizeof(char[512]));

  sj2_cli__init();

  fprintf(stderr, "Creating Tasks...\n");
  xTaskCreate(empty_task, "Empty_Task", (512U / sizeof(void *)), NULL, PRIORITY_LOW, NULL);

  xTaskCreate(mp3_reader_task, "Reader_Task", 1024, NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "Player_Task", 1024, NULL, PRIORITY_MEDIUM, NULL);

  fprintf(stderr, "Starting scheduler...\n");
  vTaskStartScheduler();
}

void end() {}