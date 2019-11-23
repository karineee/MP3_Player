#include "gpio_int.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "uart.h"

static void gpio__halt_handler(void) {

  while (1) {
  }
}

static function_pointer_t gpio0_callbacksF[32] = {
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
};

static function_pointer_t gpio0_callbacksR[32] = {
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
};

static function_pointer_t gpio2_callbacksF[32] = {
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
};

static function_pointer_t gpio2_callbacksR[32] = {
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,

    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
    gpio__halt_handler, gpio__halt_handler, gpio__halt_handler, gpio__halt_handler,
};

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {

  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
    gpio0_callbacksF[pin] = callback;
  }
  if (interrupt_type == GPIO_INTR__RISING_EDGE) {
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
    gpio0_callbacksR[pin] = callback;
  }
  NVIC_EnableIRQ(GPIO_IRQn);
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher);
}

void gpio2__attach_interupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {

  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO2IntEnF |= (1 << pin);
    gpio2_callbacksF[pin] = callback;
  }
  if (interrupt_type == GPIO_INTR__RISING_EDGE) {
    LPC_GPIOINT->IO2IntEnR |= (1 << pin);
    gpio2_callbacksR[pin] = callback;
  }
  NVIC_EnableIRQ(GPIO_IRQn);
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher);
}

static uint8_t register_bit_to_binary(uint32_t register_value) {
  uint8_t binary_value;
  // realizing that I don't need to or anything
  //(1<<X) is perfectly fine
  switch (register_value) {
  case 0x00000000 | (1 << 0):
    binary_value = 0;
    break;
  case 0x00000000 | (1 << 1):
    binary_value = 1;
    break;
  case 0x00000000 | (1 << 2):
    binary_value = 2;
    break;
  case 0x00000000 | (1 << 3):
    binary_value = 3;
    break;
  case 0x00000000 | (1 << 4):
    binary_value = 4;
    break;
  case 0x00000000 | (1 << 5):
    binary_value = 5;
    break;
  case 0x00000000 | (1 << 6):
    binary_value = 6;
    break;
  case 0x00000000 | (1 << 7):
    binary_value = 7;
    break;

  case 0x00000000 | (1 << 8):
    binary_value = 8;
    break;
  case 0x00000000 | (1 << 9):
    binary_value = 9;
    break;
  case 0x00000000 | (1 << 10):
    binary_value = 10;
    break;
  case 0x00000000 | (1 << 11):
    binary_value = 11;
    break;
  case 0x00000000 | (1 << 12):
    binary_value = 12;
    break;
  case 0x00000000 | (1 << 13):
    binary_value = 13;
    break;
  case 0x00000000 | (1 << 14):
    binary_value = 14;
    break;
  case 0x00000000 | (1 << 15):
    binary_value = 15;
    break;

  case 0x00000000 | (1 << 16):
    binary_value = 16;
    break;
  case 0x00000000 | (1 << 17):
    binary_value = 17;
    break;
  case 0x00000000 | (1 << 18):
    binary_value = 18;
    break;
  case 0x00000000 | (1 << 19):
    binary_value = 19;
    break;
  case 0x00000000 | (1 << 20):
    binary_value = 20;
    break;
  case 0x00000000 | (1 << 21):
    binary_value = 21;
    break;
  case 0x00000000 | (1 << 22):
    binary_value = 22;
    break;
  case 0x00000000 | (1 << 23):
    binary_value = 23;
    break;

  case 0x00000000 | (1 << 24):
    binary_value = 24;
    break;
  case 0x00000000 | (1 << 25):
    binary_value = 25;
    break;
  case 0x00000000 | (1 << 26):
    binary_value = 26;
    break;
  case 0x00000000 | (1 << 27):
    binary_value = 27;
    break;
  case 0x00000000 | (1 << 28):
    binary_value = 28;
    break;
  case 0x00000000 | (1 << 29):
    binary_value = 29;
    break;
  case 0x00000000 | (1 << 30):
    binary_value = 30;
    break;
  case 0x00000000 | (1 << 31):
    binary_value = 31;
    break;
  default:
    binary_value = 0;
    break;
  }
  return binary_value;
}

uint8_t gpio0_find_interrupt_pin() {
  uint32_t isr_num;

  if (LPC_GPIOINT->IO0IntStatR) {
    isr_num = LPC_GPIOINT->IO0IntStatR;

  }

  else if (LPC_GPIOINT->IO0IntStatF) {
    isr_num = LPC_GPIOINT->IO0IntStatF;
  }

  return register_bit_to_binary(isr_num);
}

uint8_t gpio2_find_interrupt_pin() {
  uint32_t isr_num;

  if (LPC_GPIOINT->IO2IntStatR) {
    isr_num = LPC_GPIOINT->IO2IntStatR;
  }

  else if (LPC_GPIOINT->IO2IntStatF) {
    isr_num = LPC_GPIOINT->IO2IntStatF;
  }

  return register_bit_to_binary(isr_num);
}

gpio_s gpio_find_interrupt_source() {
  if (LPC_GPIOINT->IntStatus & (1 << 0)) {
    return gpio__construct(0, gpio0_find_interrupt_pin());
  } else if (LPC_GPIOINT->IntStatus & (1 << 2)) {
    return gpio__construct(2, gpio2_find_interrupt_pin());
  }
}

void gpio__interrupt_dispatcher(void) {

  // check which pin generated the interrupt
  const gpio_s pin_that_generated_interrupt = gpio_find_interrupt_source();
  function_pointer_t attached_user_handler;
  if (pin_that_generated_interrupt.port_number == 0) {

    if (LPC_GPIOINT->IO0IntStatF) {
      // attach from callbacksF
      attached_user_handler = gpio0_callbacksF[pin_that_generated_interrupt.pin_number];
    } else if (LPC_GPIOINT->IO0IntStatR) {
      // attach from callbacksR
      attached_user_handler = gpio0_callbacksR[pin_that_generated_interrupt.pin_number];
    }
    attached_user_handler();
  }

  else if (pin_that_generated_interrupt.port_number == 2) {

    if (LPC_GPIOINT->IO2IntStatF) {
      // attach from callbacksF
      function_pointer_t attached_user_handler = gpio2_callbacksF[pin_that_generated_interrupt.pin_number];
    } else if (LPC_GPIOINT->IO2IntStatR) {
      // attach from callbacksR
      function_pointer_t attached_user_handler = gpio2_callbacksR[pin_that_generated_interrupt.pin_number];
    }
    attached_user_handler();
  }
}