#pragma once

#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>

typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
  GPIO_INTR__NOT_SET,
} gpio_interrupt_e;

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
// When writing handler, make sure to clear interrupt at start of handler
void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);
void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);

void gpio__interrupt_dispatcher(void);