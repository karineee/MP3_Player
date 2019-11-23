#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "acceleration.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "uart.h"
#include "uart_printf.h"

static QueueHandle_t sensor_data_queue;

xSemaphoreHandle consumer_write_signal;

EventGroupHandle_t task_flags;

void producer(void *p) {
  acceleration__axis_data_s sensor_data_buffer, sensor_average;
  EventBits_t producer_flag = (1 << 0);
  int sample_size = 100;

  while (1) {
    sensor_average.x = 0;
    sensor_average.y = 0;
    sensor_average.z = 0;

    // collect 100 samples,
    for (int i = 0; i < sample_size; i++) {
      sensor_data_buffer = acceleration__get_data();
      sensor_average.x += sensor_data_buffer.x;
      sensor_average.y += sensor_data_buffer.y;
      sensor_average.z += sensor_data_buffer.z;
    }
    sensor_average.x = sensor_average.x / sample_size;
    sensor_average.y = sensor_average.y / sample_size;
    sensor_average.z = sensor_average.z / sample_size;

    xQueueSend(sensor_data_queue, &sensor_average, portMAX_DELAY);
    xEventGroupSetBits(task_flags, producer_flag);

    vTaskDelay(1000);
  }
}

void consumer(void *p) {
  acceleration__axis_data_s sensor_data_buffer;
  EventBits_t consumer_flag = (1 << 1);

  const char *filename = "acceleration_data.txt";
  FIL file;
  UINT bytes_written = 0;
  char string[64];
  int ticks;
  while (1) {

    xQueueReceive(sensor_data_queue, &sensor_data_buffer, portMAX_DELAY);

    // for debugging
    uart_printf__polled(UART__0, "x = %d, y = %d, z = %d\n", sensor_data_buffer.x, sensor_data_buffer.y,
                        sensor_data_buffer.z);

    // todo: write to sd card
    // is the only operation doing read/writes to sd card, so we dont need a semaphore.
    // but it is good habit use locks on it.
    vTaskDelay(1000);
    FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));
    if (FR_OK == result) {
      ticks = xTaskGetTickCount();
      sprintf(string, "x = %d, y = %d, z = %d, time = %d\n", sensor_data_buffer.x, sensor_data_buffer.y,
              sensor_data_buffer.z, ticks);
      if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
      } else {
        uart_printf__polled(UART__0, "ERROR: Failed to write data to file\n");
      }
      f_close(&file);
    } else {
      uart_printf__polled(UART__0, "ERROR: Failed to open: %s\n", filename);
    }

    xEventGroupSetBits(task_flags, consumer_flag);
  }
}

void watchdog_task(void *p) {
  EventBits_t producer_flag = (1 << 0);
  EventBits_t consumer_flag = (1 << 1);
  EventBits_t triggered_flags;
  while (1) {
    vTaskDelay(1000);
    triggered_flags = xEventGroupWaitBits(task_flags, producer_flag | consumer_flag, pdTRUE, pdFALSE, 0);

    if (triggered_flags == (producer_flag | consumer_flag)) {
      // both flags triggered
      uart_printf__polled(UART__0, "Both Task Ok!\n");

    } else if (triggered_flags == consumer_flag) {
      // producer flag was not set
      uart_printf__polled(UART__0, "Producer NOT ok\n");
    } else if (triggered_flags == producer_flag) {
      // consumer flag was not set
      uart_printf__polled(UART__0, "Consumer NOT ok\n");
    } else {
      // nothing was set
      uart_printf__polled(UART__0, "Producer and Consumer NOT ok\n");
    }

    xEventGroupClearBits(task_flags, producer_flag | consumer_flag);
  }
}

int main7(void) {
  uart0_init();

  consumer_write_signal = xSemaphoreCreateBinary();
  sensor_data_queue = xQueueCreate(10, sizeof(acceleration__axis_data_s));
  task_flags = xEventGroupCreate();

  if (task_flags != NULL) { // Event Group creation success

    sj2_cli__init();
    acceleration__init();

    TaskHandle_t prod, cons, watchdog;
    xTaskCreate(producer, "Producer", (512U * 4) / sizeof(void *), NULL, 2, &prod);
    xTaskCreate(consumer, "Consumer", (512U * 4) / sizeof(void *), NULL, 2, &cons);
    xTaskCreate(watchdog_task, "Watchdog", (512U * 4) / sizeof(void *), NULL, 3, &watchdog);
    vTaskStartScheduler();
  } else {
    uart_printf__polled(UART__0, "Event Group Creation Failed\n");
  }
}
