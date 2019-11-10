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

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  char name[32];
  char bytes_512[512];
  char bytes_512_2[512];

  printf("Inside the mp3_reader_task \n");

  while (1) {
    //TODO: IMPLEMENT THESE TWO LINES
    // xQueueReceive(Q_songname, &name, portMAX_DELAY);
    // printf("Received song to play: %s\n", name);

    const char *testName = "test.mp3";
    printf("Testing: %s\n", testName);

    FILE mp3File; // File handle
    FRESULT result = f_open(&mp3File, testName, (FA_READ));

    if (FR_OK == result) {
      printf("File Found: %s\n", testName);
      printf("Begin Queueing File Data!\n\n");

      // char c = fpeek(&mp3File);
      // printf("Just peeked: %c\n");

      int iteration = 0;
      char buffer[512];
      // while (c != EOF) {
      while (buffer[511] != 130) { // should be while(not eof)

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
          // printf("# %d: %c\n", i + (iteration * 512), c);
          printf("%c", buffer[i]);
        }

        //**********************

        printf("\nBlock #%d read\n", iteration);
        vTaskDelay(2000);
        // read_from_file(bytes_512_2, mp3File);
        // printf("Just finished reading bytes 2\n");
        // vTaskDelay(5000);
        xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
        printf("Just sent 512 bytes to Q_songdata!\n");
        // char c = fpeek(mp3File);
        vTaskDelay(2000);
        iteration++;
        printf("buffer[511]: %c, %i\n", buffer[511], buffer[511]);
      }

      f_close(&mp3File);
      printf("We read the file and reached the end\n");
      vTaskDelay(5000);
    } else {
      printf("ERROR: Failed to open: %s\n", testName);
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
    printf("# %d: %c\n", i, c);
  }

  printf("read_from_file: Just placed 512 bytes into buffer\n");
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

void main(void) {
  Q_songname = xQueueCreate(sizeof(char[32]), 1);
  Q_songdata = xQueueCreate(512, 1);

  printf("Creating Tasks...\n");
  xTaskCreate(mp3_reader_task, "Reader_Task", (2048U * 8 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  // xTaskCreate(producer, "Producer", (512U / sizeof(void *)), NULL, PRIORITY_LOW, NULL);

  printf("Starting scheduler...\n");
  vTaskStartScheduler();
}

void end() {}