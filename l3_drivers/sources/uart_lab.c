#include "uart_lab.h"
#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

#include "uart.h"
#include "uart_printf.h"

// U0_Tx connected to USB Device, don't use unless communicating to PC
// U0_Rx

// U1_Tx, P0_15
// U1_Rx, P0_16

// U2_Tx, P0_10, [P2_8]
// U2_Rx, P0_11, [P2_9]

// U3_Tx, P0_0, [P4_28], P0_25
// U3_Rx, P0_1, [P4_29], P0_26

// U4_Tx, P4_28
// U4_Rx, P4_29

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly

  uint16_t divisor = (uint16_t)(peripheral_clock / (16 * baud_rate));
  const uint32_t DLAB_mask = (1 << 7);

  if (uart == UARTL__2) {
    const uint32_t uart2_power_bit = (1 << 24);
    LPC_SC->PCONP |= uart2_power_bit;
    LPC_UART2->LCR = 3; // for 8-bit data

    LPC_UART2->LCR |= DLAB_mask;

    LPC_UART2->DLM = (divisor >> 8) & 0xFF;
    LPC_UART2->DLL = (divisor >> 0) & 0xFF;
    LPC_UART2->FDR = (1 << 4);

    LPC_UART2->LCR &= ~DLAB_mask;

  } else if (uart == UARTL__3) {
    const uint32_t uart3_power_bit = (1 << 25);
    LPC_SC->PCONP |= uart3_power_bit;
    LPC_UART3->LCR = 3;

    LPC_UART3->LCR |= DLAB_mask;

    LPC_UART3->DLM = (divisor >> 8) & 0xFF;
    LPC_UART3->DLL = (divisor)&0xFF;
    LPC_UART3->FDR = (1 << 4);

    LPC_UART3->LCR &= ~DLAB_mask;
  }
}

void uart_pinsel_init(uart_number_e uart) {

  if (uart == UARTL__2) {
    gpio__construct_as_output(GPIO__PORT_2, 8);
    gpio__construct_with_function(GPIO__PORT_2, 8, GPIO__FUNCTION_2); // Tx

    gpio__construct_as_input(GPIO__PORT_2, 9);
    gpio__construct_with_function(GPIO__PORT_2, 9, GPIO__FUNCTION_2); // Rx
  }

  else if (uart == UARTL__3) {
    gpio__construct_as_output(GPIO__PORT_4, 28);
    gpio__construct_with_function(GPIO__PORT_0, 28, GPIO__FUNCTION_2); // Tx

    gpio__construct_as_input(GPIO__PORT_4, 29);
    gpio__construct_with_function(GPIO__PORT_0, 29, GPIO__FUNCTION_2); // Rx
  }
}

bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
  if (uart == UARTL__2) {
    while (!(LPC_UART2->LSR & (1 << 0))) {
    } // wait until ready with data

    *input_byte = LPC_UART2->RBR;
  }

  else if (uart == UARTL__3) {
    while (!(LPC_UART3->LSR & (1 << 0))) {
    } // wait until ready with data

    *input_byte = LPC_UART3->RBR;
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register
  if (uart == UARTL__2) {
    while (!(LPC_UART2->LSR & (1 << 5))) {
    } // wait until transmit hold register is  empty
    LPC_UART2->THR = output_byte;
  }

  else if (uart == UARTL__3) {
    while (!(LPC_UART3->LSR & (1 << 5))) {
    } // wait until empty
    LPC_UART3->THR = output_byte;
  }
}

static void uart__halt_handler(void) {

  while (1) {
  }
}

static function_pointer_t uart2_callbacks[2] = {uart__halt_handler, uart__halt_handler};

static function_pointer_t uart3_callbacks[2] = {uart__halt_handler, uart__halt_handler};

void uart_interrupt_dispatcher(void) {
  // check who trigged interrupt???

  function_pointer_t attached_user_handler;
  if (!(LPC_UART2->IIR & (1 << 0))) {
    if (LPC_UART2->IIR & (2 << 1)) {
      attached_user_handler = uart2_callbacks[0];

    } else if (LPC_UART2->IIR & (1 << 1)) {
      attached_user_handler = uart2_callbacks[1];
    }
  }

  else if (!(LPC_UART3->IIR & (1 << 0))) {
    if (LPC_UART3->IIR & (2 << 1)) {
      attached_user_handler = uart3_callbacks[0];
    } else if (LPC_UART3->IIR & (1 << 1)) {
      attached_user_handler = uart3_callbacks[1];
    }
  }
  attached_user_handler();
}

void uart_lab_attach_interrupt(uart_number_e uart, uart_int_type int_type, function_pointer_t isr_callback) {
  if (uart == UARTL__2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, uart_interrupt_dispatcher);
    // activate inputted uart_int_type
    const uint32_t DLAB_mask = (1 << 7);
    if (int_type == receive_intr) {
      LPC_UART2->LCR &= ~(DLAB_mask);
      LPC_UART2->IER |= (1 << 0);
      uart2_callbacks[0] = isr_callback;
    } else if (int_type == transmit_intr) {
      LPC_UART2->LCR &= ~(DLAB_mask);
      LPC_UART2->IER |= (1 << 1);
      uart2_callbacks[1] = isr_callback;
    }
    NVIC_EnableIRQ(UART2_IRQn);
  } else if (uart == UARTL__3) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, uart_interrupt_dispatcher);
    // activate inputted uart_int_type
    const uint32_t DLAB_mask = (1 << 7);
    if (int_type == receive_intr) {
      LPC_UART3->LCR &= ~(DLAB_mask);
      LPC_UART3->IER |= (1 << 0);
      uart3_callbacks[0] = isr_callback;
    } else if (int_type == transmit_intr) {
      LPC_UART3->LCR &= ~(DLAB_mask);
      LPC_UART3->IER |= (1 << 1);
      uart3_callbacks[1] = isr_callback;
    }
    NVIC_EnableIRQ(UART3_IRQn);
  }
}
