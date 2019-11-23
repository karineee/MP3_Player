// Emil Agonoy
// Working Main file

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "adc.h"
#include "gpio.h"
#include "gpio_int.h"
#include "lpc40xx.h"
#include "pwm1.h"

#include "board_io.h"
#include "delay.h"
#include "uart.h"
#include "uart_printf.h"

QueueHandle_t adc_value_queue;

typedef enum { RED, GREEN, BLUE } color_state;

color_state state;

void intr_red() {
  LPC_GPIOINT->IO0IntClr |= (1 << 29);
  state = RED;
}

void intr_green() {
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
  state = GREEN;
}

void intr_blue() {
  LPC_GPIOINT->IO0IntClr |= (1 << 17);
  state = BLUE;
}

void RGB_control_init(void) {

  // initialize color state
  // initialize sw 3, 2, 1 as interrupts
  // attache gpio interrupt
  state = RED;

  gpio_s sw3 = gpio__construct_as_input(0, 29);
  gpio_s sw2 = gpio__construct_as_input(0, 30);
  gpio_s swE = gpio__construct_as_input(0, 17);

  gpio0__attach_interrupt(29, GPIO_INTR__FALLING_EDGE, intr_red);
  gpio0__attach_interrupt(30, GPIO_INTR__FALLING_EDGE, intr_green);
  gpio0__attach_interrupt(17, GPIO_INTR__FALLING_EDGE, intr_blue);
}

float adc_to_pwm_duty_cycle(uint16_t adc_value) {
  float result = (float)adc_value / 4095;
  result = result * 100;
  return result;
}

void read_adc(void *p) {
  adc__initialize();

  adc__configure_io_pin(ADC__CHANNEL_2);

  adc__enable_burst_mode();

  uint16_t adc_reading = 0;

  while (1) {

    adc_reading = adc__get_adc_value_bm(ADC__CHANNEL_2);

    xQueueSend(adc_value_queue, &adc_reading, 0);
    vTaskDelay(100);
  }
}

void convert_adc_to_pwm(void *p) {
  gpio_s pwm_red = gpio__construct(2, 0);
  gpio__set_function(pwm_red, GPIO__FUNCTION_1);

  gpio_s pwm_green = gpio__construct(2, 1);
  gpio__set_function(pwm_green, GPIO__FUNCTION_1);

  gpio_s pwm_blue = gpio__construct(2, 2);
  gpio__set_function(pwm_blue, GPIO__FUNCTION_1);

  pwm1__init_single_edge(1000);

  float percent;
  uint16_t adc_reading;
  percent = adc_to_pwm_duty_cycle(adc_reading);

  while (1) {
    if (xQueueReceive(adc_value_queue, &adc_reading, 100)) {
      percent = adc_to_pwm_duty_cycle(adc_reading);

      if (state == RED) {
        pwm1__set_duty_cycle(PWM1__2_0, percent);
      } else if (state == GREEN) {
        pwm1__set_duty_cycle(PWM1__2_1, percent);

      } else if (state == BLUE) {
        pwm1__set_duty_cycle(PWM1__2_2, percent);
      }
    }
  }
}

int main3(void) {
  adc_value_queue = xQueueCreate(1, sizeof(int));
  RGB_control_init();
  uart_printf__polled(UART__0, "STARTING ADC CONTROL");
  xTaskCreate(read_adc, "read_adc", (512U * 4) / sizeof(void *), NULL, 1, NULL);
  xTaskCreate(convert_adc_to_pwm, "set_pwm", (512U * 4) / sizeof(void *), NULL, 1, NULL);
  vTaskStartScheduler();

  return 0;
}