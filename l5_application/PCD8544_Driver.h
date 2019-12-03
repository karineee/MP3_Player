#ifndef LCD_Pins.h
#define LCD_Pins .h

#include "gpio.h"
#include "lpc40xx.h"
#include "ssp_lab.h"
#include <stdio.h>

// RESET = P2_0
// BACKLIGHT = P2_1

// SCE = Chip Enable (Active Low)
// DC = Data or Command
// SDIN = Serial Data In
// SCLK = Serial Clock

typedef struct {
  uint8_t x;
  uint8_t y;
} cursor;

void init_lcd();

void reset_pulse();

void clear_lcd();

void config_lcd_pins() {
  gpio__construct_as_output(1, 28); // RESET
  gpio__construct_as_output(1, 29); // SCE
  gpio__construct_as_output(1, 30); // DC
  // gpio__construct_as_output(2, 3); // SDIN (MOSI)
  gpio__construct_as_output(1, 31); // SCLK

  LPC_GPIO1->SET = (1 << 28); // set RESET
  LPC_GPIO1->CLR = (1 << 29); // clear SCE (pg 11)

  for (int i = 0; i < 999; i++) {
  } // waiting for LCD to recognize pins

  fprintf(stderr, "config_lcd_pins() complete\n");
}

void reset_pulse() {
  LPC_GPIO1->CLR = (1 << 28); // clear RESET, setting it active

  for (int i = 0; i < 9999; i++) {
  } // delay so that RESET can be registered on lcd

  LPC_GPIO1->SET = (1 << 28);

  fprintf(stderr, "reset_pulse() complete\n");
}

void spi1_init() {
  LPC_SC->PCONP |= (1 << 10);
  LPC_SC->PCLKSEL |= 1;
  LPC_SSP1->CPSR = sck_init(24);

  LPC_IOCON->P0_6 |= 0;        // CS1
  LPC_IOCON->P0_7 |= (4 << 0); // SCK1
  LPC_IOCON->P0_8 |= (4 << 0); // MISO1
  LPC_IOCON->P0_9 |= (4 << 0); // MOSI1

  LPC_GPIO0->DIR &= ~(1 << 6);

  LPC_SSP1->CR0 = (7 << 0);
  LPC_SSP1->CR1 = (1 << 1);

  fprintf(stderr, "spi1_init() complete\n");
}

void set_display_mode_normal() {
  LPC_GPIO1->CLR = (1 << 30); // set D/C pin to 0, so we can write a command
  for (int i = 0; i < 500; i++) {
  } // delay so that LCD can recognize command input

  // display control function
  // DB3 = 1 == 0000 1XXX
  // DB2 = D (1 for "normal display mode")
  // DB1 = 0
  // DB0 = E (0 for "normal display mode")
  // so bytes should be 0000 1100
  // which is 0x0C in hex

  ssp1__exchange_byte(0x0C); // command for "display mode = normal"

  for (int i = 0; i < 500; i++) {
  } // delay so that command is guaranteed to be sent

  LPC_GPIO1->SET = (1 << 30); // set D/C pin to 1, so normal data can be exchanged

  for (int i = 0; i < 500; i++) {
  } // delay so that new D/C value is guaranteed to be recognized by LCD

  fprintf(stderr, "set_display_mode_normal() complete\n");
}

void lcd_putchar(char in, cursor pos) {

  LPC_GPIO1->SET = (1 << 30); // enabling data transfer

  switch (in) {
  case 'a':

    break;

  case 'H':
    // 0xFE, 0x10, 0x10, 0x10, 0xFE, 0x00,
    ssp1__exchange_byte(0xFE);
    // increment_cursor(pos);
    ssp1__exchange_byte(0x10);
    // increment_cursor(pos);
    ssp1__exchange_byte(0x10);
    // increment_cursor(pos);
    ssp1__exchange_byte(0x10);
    // increment_cursor(pos);
    ssp1__exchange_byte(0xFE);
    // increment_cursor(pos);
    ssp1__exchange_byte(0x00);
    fprintf(stderr, "In lcd_putchar(), just sent 'H'\n");

    break;

  case 'I':
    // 0x82, 0x82, 0xFE, 0x82, 0x82, 0x00,
    ssp1__exchange_byte(0x82);
    ssp1__exchange_byte(0x82);
    ssp1__exchange_byte(0xFE);
    ssp1__exchange_byte(0x82);
    ssp1__exchange_byte(0x82);
    ssp1__exchange_byte(0x00);
    break;
  }
}

void increment_cursor(cursor pos) {
  pos.x++;

  if (pos.x >= 83) {
    pos.x = 0;
    // TODO:
    // increment y
    // send new yPos
  }

  LPC_GPIO1->CLR = (1 << 30); // enabling command transfer

  for (int i = 0; i < 500; i++) {
  } // delay so change in D/C pin is recognized by LCD

  // to set X address of cursor (pg 14)
  // DB7 is 1, DB6 thru DB0 are the bits of the X position
  // so value should be:   1 X6 X5 X4   X3 X2 X1 X0

  uint16_t send = 0;
  send |= 0b10000000;
  send |= pos.x;

  ssp1__exchange_byte(send);

  LPC_GPIO1->SET = (1 << 30); // re-enable data transfer
}

void clear_display_ram(cursor pos) {
  reset_pulse();
  set_display_mode_normal();

  pos.x = 0;
  pos.y = 0;
}

void set_initial_cursor_positions(cursor pos) {
  pos.x = 0;
  pos.y = 0;

  LPC_GPIO1->CLR = (1 << 30); // enabling command transfer

  for (int i = 0; i < 500; i++) {
  } // delay so change in D/C pin is recognized by LCD

  // Sending X position (pg 14)
  // DB7 is 1, DB6 thru DB0 are the bits of the X position
  // so value should be:   1 X6 X5 X4   X3 X2 X1 X0
  // in the initial position, x should be zero, so we set those to zero

  uint16_t send = 0;
  send = 0b10000000;

  ssp1__exchange_byte(send);

  // Sending Y position (pg 14)
  // DB6 should be 1
  // DB2 = Y2, DB1 = Y1, DB0 = Y0, but initially, all Y values are zero

  send = 0b01000000;
  ssp1__exchange_byte(send);
  fprintf(stderr, "Set initial cursor positions\n");
}

void function_set() {
  LPC_GPIO1->CLR = (1 << 30); // enabling command mode

  ssp1__exchange_byte(0b00100001); // pg 22 step 2
  ssp1__exchange_byte(0b10010000); // pg 22 step 3
  // function set is:
  // 0010   0 PD V H, where...
  //      PD = 0 means chip is active, PD=1 means power down mode
  //          we're setting the chip active, so PD = 0
  //      V = 0 means horizontal addressing, V = 1 means vertical addressing
  //          V = 0 (pg 22)
  //      H = 0 means basic instruction set, H = 1 means extended instruction set
  //          we're using the basic instruction set, so H = 0
  uint8_t send = 0b00100000;

  fprintf(stderr, "send was %d, should have been 32\n", send);

  ssp1__exchange_byte(send);

  fprintf(stderr, "function_set() complete\n");
}

void init_lcd(cursor pos) {
  spi1_init();
  config_lcd_pins();
  reset_pulse();             // step 8.1/8.2 on pg 15
  function_set();            // step 8.3 on pg 15
  set_display_mode_normal(); // step 8.4 on pg 15
  // send_initial_cursor_positions(pos); // step 8.5 and 8.6

  fprintf(stderr, "LCD Setup Complete!\n");
}

#endif
