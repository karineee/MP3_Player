#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"
//#include "i2c.h"
#include "i2c_slave.h"
#include "lpc40xx.h"
#include "pwm1.h"
//#include "sj2_cli.h"

static void blink_task(void *params);
static void blink_on_startup(gpio_s gpio, int count);
static void RGB_mux(uint8_t select);

static gpio_s led0, led1, ledR, ledG, ledB;

volatile uint8_t memory[256] = {0};

static void rgb_control_task(void *params) {

  ledR = gpio__construct_as_output(GPIO__PORT_2, 0);
  ledG = gpio__construct_as_output(GPIO__PORT_2, 1);
  ledB = gpio__construct_as_output(GPIO__PORT_2, 2);

  while (true) {
    if (memory[0] & (1 << 0)) {
      if (memory[0] & (1 << 1)) {
        gpio__set(ledR);
      } else {
        gpio__reset(ledR);
      }
      if (memory[0] & (1 << 2)) {
        gpio__set(ledG);
      } else {
        gpio__reset(ledG);
      }
      if (memory[0] & (1 << 3)) {
        gpio__set(ledB);
      } else {
        gpio__reset(ledB);
      }
    } else {
      RGB_mux(memory[1]);
    }
    vTaskDelay(100);
  }
}

int main123(void) {
  gpio_s ledR, ledG, ledB;
  ledR = gpio__construct_as_output(GPIO__PORT_2, 0);
  // pin set up for master and slave devices

  gpio__construct_with_function(GPIO__PORT_0, 10, GPIO__FUNCTION_2);
  gpio__construct_with_function(GPIO__PORT_0, 11, GPIO__FUNCTION_2);
  LPC_IOCON->P0_10 &= ~(3 << 3);
  LPC_IOCON->P0_11 &= ~(3 << 3);
  LPC_IOCON->P0_10 |= (1 << 10);
  LPC_IOCON->P0_11 |= (1 << 10);

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  blink_on_startup(led1, 2);

  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  i2c__initialize_slave(I2C__2s, i2c_speed_hz, clock__get_peripheral_clock_hz(), 0x42, memory);

  // configMINIMAL_STACK_SIZE
  xTaskCreate(blink_task, "led0", 1024, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", 1024, (void *)&led1, PRIORITY_LOW, NULL);
  xTaskCreate(rgb_control_task, "ledR", 1024, NULL, PRIORITY_LOW, NULL);

  // sj2_cli__init();

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails
  return 0;
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params);
  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    // gpio__toggle(led);

    if (memory[0]) {
      gpio__toggle(led);
    } else {
      gpio__set(led);
    }

    vTaskDelay(500);
  }
}

static void blink_on_startup(gpio_s gpio, int blinks) {
  const int toggles = (2 * blinks);
  for (int i = 0; i < toggles; i++) {
    delay__ms(250);
    gpio__toggle(gpio);
  }
}

static void RGB_mux(uint8_t select) {

  switch (select) {
  case (1):
    gpio__set(ledR);
    gpio__reset(ledG);
    gpio__reset(ledB);
    break;

  case (2):
    gpio__reset(ledR);
    gpio__set(ledG);
    gpio__reset(ledB);
    break;

  case (3):
    gpio__set(ledR);
    gpio__set(ledG);
    gpio__reset(ledB);
    break;

  case (4):
    gpio__reset(ledR);
    gpio__reset(ledG);
    gpio__set(ledB);
    break;

  case (5):
    gpio__set(ledR);
    gpio__reset(ledG);
    gpio__set(ledB);
    break;

  case (6):
    gpio__reset(ledR);
    gpio__set(ledG);
    gpio__set(ledB);
    break;

  case (7):
    gpio__set(ledR);
    gpio__set(ledG);
    gpio__set(ledB);
    break;

  case (0):
  default:
    gpio__reset(ledR);
    gpio__reset(ledG);
    gpio__reset(ledB);
    break;
  }
}
