
#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdbool.h>

uint32_t sck_init(uint32_t max_clock_mhz) {
  uint32_t clock = clock__get_core_clock_hz();

  uint8_t prescalar = 2;
  clock = clock / 2;
  while (max_clock_mhz < clock && prescalar <= 254) {
    prescalar += 2;
    clock = clock / 2;
  }
  return (prescalar);
}

// initialize SPI2
void spi2_init() {

  LPC_SC->PCONP |= (1 << 20);
  LPC_SC->PCLKSEL |= 1;
  LPC_SSP2->CPSR = sck_init(24);

  LPC_IOCON->P1_1 |= (4 << 0); // MOSI2
  LPC_IOCON->P1_0 |= (4 << 0); // SCK2
  LPC_IOCON->P1_4 |= (4 << 0); // MISO2
  LPC_IOCON->P1_8 |= 0;        // CS2
  LPC_GPIO1->DIR &= ~(1 << 8);
  // User manual control registers CR0 and CR1
  LPC_SSP2->CR0 = (7 << 0); // For 8-bit mode, setting 0111 to bits 3:0
  LPC_SSP2->CR1 = (1 << 1); // Enable SSP as Master
}

// initialize SPI0

void spi0_init() {
  LPC_SC->PCONP |= (1 << 21);
  LPC_SC->PCLKSEL |= 1;
  LPC_SSP0->CPSR = sck_init(24);
  // fix w schem
  LPC_IOCON->P1_24 |= (4 << 0); // MOSI0
  LPC_IOCON->P1_20 |= (4 << 0); // SCK0
  LPC_IOCON->P1_23 |= (4 << 0); // MISO0

  // control select
  LPC_IOCON->P1_4 |= 0; // CS0
  LPC_GPIO1->DIR &= ~(1 << 4);

  // data select
  LPC_IOCON->P1_28 |= 0; // DS
  LPC_GPIO1->DIR &= ~(1 << 28);

  // data request
  LPC_IOCON->P2_0 |= 0; // DS
  LPC_GPIO1->DIR &= ~(1 << 20);

  // User manual control registers CR0 and CR1
  LPC_SSP0->CR0 = (15 << 0); // For 8-bit mode, setting 0111 to bits 3:0
  LPC_SSP0->CR1 = (1 << 1);  // Enable SSP as Master
}

bool get_data_request_signal() {

  bool signal;
  signal = LPC_GPIO1->PIN & (1 << 20);

  return (signal);
}

void sp0_cs() {

  LPC_GPIO1->CLR = (4 << 1);  // clr
  LPC_GPIO1->SET = (28 << 1); // set
}
void sp0_ds() {

  LPC_GPIO1->CLR = (28 << 1); // clr
  LPC_GPIO1->SET = (4 << 1);  // set
}

uint8_t ssp2_exchange_byte(uint8_t byte_to_transmit) {
  LPC_SSP2->DR = byte_to_transmit;

  while (LPC_SSP2->SR & (1 << 4)) {
    ; // Wait until SSP is busy
  }
  return (uint8_t)(LPC_SSP2->DR & 0xFF);
}
uint8_t ssp0_exchange_byte(uint8_t byte_to_transmit) {
  LPC_SSP0->DR = byte_to_transmit;

  while (LPC_SSP0->SR & (1 << 4)) {
    ; // Wait until SSP is busy
  }
  return (uint8_t)(LPC_SSP0->DR & 0xFF);
}

//