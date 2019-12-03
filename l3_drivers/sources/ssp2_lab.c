#include <stdbool.h>
#include <stddef.h>

#include "ssp2_lab.h"

#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

/* SSP2
 * SCK:  P1_0
 * MOSI: P1_1
 * MISO: P1_4
 * CS_flash: P1_10
 * CS_SD: P1_8
 */

uint32_t sck_init(uint32_t max_clock) {

  // does not assume any ideal value of core clock
  uint32_t clock = clock__get_core_clock_hz();

  uint8_t prescaler = 2;
  clock = clock / 2;

  while (max_clock < clock && prescaler <= 254) {
    prescaler += 2;
    clock = clock / 2;
  }

  return prescaler;
}
void ssp2__init(uint32_t max_clock_mhz) {
  LPC_SC->PCONP |= (1 << 20); // power on peripheral

  LPC_SSP2->CR0 = (7 << 0); // Control Register Setup
  LPC_SSP2->CR1 = (1 << 1);

  LPC_SSP2->CPSR = sck_init(max_clock_mhz);

  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCTION_4);
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCTION_4);
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCTION_4);
}

void ssp0__init(uint32_t max_clock_mhz) {
  LPC_SC->PCONP |= (1 << 21); // power on peripheral

  LPC_SSP0->CR0 = (7 << 0); // Control Register Setup
  LPC_SSP0->CR1 = (1 << 1);

  LPC_SSP0->CPSR = sck_init(max_clock_mhz);

  gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);
}

void ssp2__select_SD();
uint8_t ssp2L__exchange_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the status register
  LPC_SSP2->DR = data_out;

  while (LPC_SSP2->SR & (1 << 4)) {
  } // read SR Busy bit

  return (uint8_t)LPC_SSP2->DR & 0xFF;
}

uint8_t ssp0__exchange_byte(uint8_t data_out) {
  LPC_SSP0->CR1 = 0;
  LPC_SSP0->CR0 = (7 << 0);
  LPC_SSP0->CR1 = (1 << 1);
  LPC_SSP0->DR = data_out;

  while (LPC_SSP0->SR & (1 << 4)) {
  } // read SR Busy bit

  return (uint8_t)LPC_SSP0->DR & 0xFF;
}

uint16_t ssp0__exchange_half(uint16_t data_out) {
  LPC_SSP0->CR1 = 0;
  LPC_SSP0->CR0 = (15 << 0);
  LPC_SSP0->CR1 = (1 << 1);
  LPC_SSP0->DR = data_out;

  while (LPC_SSP0->SR & (1 << 4)) {
  } // read SR Busy bit

  return (uint16_t)LPC_SSP0->DR & 0xFFFF;
}

uint8_t ssp1__exchange_byte(uint8_t data_out) {
  LPC_SSP1->CR1 = 0;
  LPC_SSP1->CR0 = (7 << 0);
  LPC_SSP1->CR1 = (1 << 1);
  LPC_SSP1->DR = data_out;

  while (LPC_SSP1->SR & (1 << 4)) {
    fprintf(stderr, "waiting on SR busy bit...\n");
  } // read SR Busy bit

  return (uint8_t)LPC_SSP1->DR & 0xFF;
}

uint16_t ssp1__exchange_half(uint16_t data_out) {
  LPC_SSP1->CR1 = 0;
  LPC_SSP1->CR0 = (15 << 0);
  LPC_SSP1->CR1 = (1 << 1);
  LPC_SSP1->DR = data_out;

  while (LPC_SSP1->SR & (1 << 4)) {
  } // read SR Busy bit

  return (uint16_t)LPC_SSP1->DR & 0xFFFF;
}
