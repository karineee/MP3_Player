#pragma once

#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  UARTL__2 = 2,
  UARTL__3 = 3,
} uart_number_e;

typedef enum {
  receive_intr = 0,
  transmit_intr = 1,
} uart_int_type;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);

void uart_pinsel_init(uart_number_e uart);

bool uart_lab__polled_get(uart_number_e uart, char *input_byte);

bool uart_lab__polled_put(uart_number_e uart, char output_byte);

typedef void (*function_pointer_t)(void);
void uart_lab_attach_interrupt(uart_number_e uart, uart_int_type int_type, function_pointer_t isr_callback);

void uart_interrupt_dispatcher(void);