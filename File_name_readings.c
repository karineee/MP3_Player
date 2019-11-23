#include "FreeRTOS.h"
#include "acceleration.h"
#include "board_io.h"
#include "common_macros.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "sensors.h"
#include "sj2_cli.h"
#include "sl_string.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static void blink_task(void *params);
static void uart_task(void *params);

static QueueHandle_t sensor_queue;
EventGroupHandle_t xEventGroup;
EventBits_t xEventBits;

static gpio_s led0, led1;

FRESULT scan_files(char *path /* Start node to be scanned (***also used as work area***) */
) {
  FRESULT res;
  DIR dir;
  UINT i;
  static FILINFO fno;

  res = f_opendir(&dir, path); /* Open the directory */
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno); /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0)
        break;                    /* Break on error or end of dir */
      if (fno.fattrib & AM_DIR) { /* It is a directory */
        i = strlen(path);
        sprintf(&path[i], "/%s", fno.fname);
        res = scan_files(path); /* Enter the directory */
        if (res != FR_OK)
          break;
        path[i] = 0;
      } else { /* It is a file. */
        printf("%s/%s\n", path, fno.fname);
      }
    }
    f_closedir(&dir);
  }

  return res;
}

// TASK
void watchdog_task(void *params) {
  // EventBits_t bitsToWaitFor = (xEventGroup, (1 << 1) | (1 << 2), pdTRUE, pdTRUE, 1000);
  // xEventGroupWaitBits(event group, bits to look at, clearOnExit, waitForAll, ticksToWait)

  while (true) {

    vTaskDelay(1000);
    if (xEventGroupWaitBits(xEventGroup, (1 << 1) | (1 << 2), pdTRUE, pdTRUE, 1000)) {
      xEventGroupClearBits(xEventGroup, (1 << 1) | (1 << 2));
      // printf("All bits were found by the watchdog, and have been cleared.\n");
    } else {
      // printf("Not all bits were set after 1000 ticks!\n");
    }
  }
}

// SENSOR TASK

static void producer_collect_accelerometer_readings_task(void *params) {
  int readingCount = 0;
  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    acceleration__axis_data_s myData[100];
    myData[readingCount] = acceleration__get_data();

    /*
        printf("Acceleration Data: \n");
        printf("x: %d\n", myData.x);
        printf("y: %d\n", myData.y);
        printf("z: %d\n", myData.z);
    */

    readingCount++;

    if (readingCount >= 100) {
      // printf("100 Samples collected! Now computing the average... \n");
      int xTotal = 0, yTotal = 0, zTotal = 0;

      for (int i = 0; i < 100; i++) {
        xTotal += myData[i].x;
        yTotal += myData[i].y;
        zTotal += myData[i].z;
      }

      int xAvg = xTotal / 100;
      int yAvg = yTotal / 100;
      int zAvg = zTotal / 100;

      // printf("The average x value: %d\n", xAvg);
      // printf("The average y value: %d\n", yAvg);
      // printf("The average z value: %d\n", zAvg);

      FATFS fs;
      FRESULT res;
      char buff[256];

      acceleration__axis_data_s avgData;
      avgData.x = xAvg;
      avgData.y = yAvg;
      avgData.z = zAvg;

      xQueueSend(sensor_queue, &xAvg, 0);
      // xQueueSend(sensor_queue, avgData.y, 0);
      // xQueueSend(sensor_queue, avgData.z, 0);

      vTaskDelay(2500);
      readingCount = 0;
      xEventGroupSetBits(xEventGroup, (1 << 1));
    }

    // vTaskDelay(50);
  }
}

// FUNCTION
void write_file_using_fatfs_pi(int valToWrite, FIL file, FRESULT result) {
  const char *filename = "sensor.txt";

  UINT bytes_written = 2;

  int time = xTaskGetTickCount();

  // FRESULT result = f_open(&file, "sensor.txt", (FA_WRITE | FA_CREATE_ALWAYS));
  if (FR_OK == result) {
    char string[64] = "";
    // printf("Right before sprintf, X Average: %i\n", valToWrite);
    printf("Right before sprintf, Time: %i, X val: %i\n", time, valToWrite);

    // sprintf(string, "X Average: %i\n", valToWrite);
    sprintf(string, "Time: %i, X val: %i\n", time, valToWrite);

    printf("After sprintf, string = ");
    printf(string);
    printf("\n");

    result = f_lseek(&file, feof);

    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
      printf("writing to the file: success\n");
    } else {
      printf("ERROR: Failed to write data to file\n");
    }

    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
} // Task at PRIORITY_MEDIUM

// CONSUMER TASK
void consumer(void *p) {
  printf("Consumer task starting  \n ");
  int x;
  char string[64] = "helloooo";
  FIL file; // File handle
            // FRESULT result = f_open(&file, "sensor.txt", (FA_WRITE | FA_OPEN_APPEND));

  FATFS fs;
  FRESULT res;
  char buff[256];

  res = f_mount(&fs, "", 1);
  if (res == FR_OK) {
    strcpy(buff, "/");
    res = scan_files(buff);
  }

  // write_file_using_fatfs_pi(123, file, result);
  while (1) {
    // Printing a message before xQueueReceive()
    printf(" \n");
    // printf("Before receiving \n ");
    xQueueReceive(sensor_queue, &x, portMAX_DELAY); // Printing a message after xQueueReceive()
    // printf("CONSUMER: received %d from queue \n", x);

    // write_file_using_fatfs_pi(x, file, result);

    xEventGroupSetBits(xEventGroup, (1 << 2));
  }
}

int main(void) {
  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  sj2_cli__init();
  sensors__init();

  sensor_queue = xQueueCreate(10, sizeof(int));
  xEventGroup = xEventGroupCreate();
  if (xEventGroup == NULL) {
    printf("EVENT GROUP NOT CREATED, ABORTING!\n");
    return 0;
  } else {
    printf("Event Group succesfully created.\n");
  }

  xTaskCreate(producer_collect_accelerometer_readings_task, "Producer", 1024U, (void *)&led0, PRIORITY_MEDIUM, NULL);
  xTaskCreate(consumer, "Consumer", 1024U, (void *)&led1, PRIORITY_MEDIUM, NULL);
  xTaskCreate(watchdog_task, "Watchdog", 512U, (void *)&led1, PRIORITY_HIGH, NULL);

  printf("Starting Scheduler...\n");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}
