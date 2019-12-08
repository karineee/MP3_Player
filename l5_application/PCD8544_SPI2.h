#ifndef PCD8544_SPI2_H
#define PCD8544_SPI2_H

#include "gpio.h"
#include "lpc40xx.h"
#include "ssp_lab.h"
#include <ctype.h>
#include <stdio.h>
#include <time.h>

// TODO:
//      -Complete Alphabet Library
//      -Rewrite to work for SSP0
//      -lcd_putString() function

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
  // gpio__construct_as_output(1, 31); // SCLK

  LPC_GPIO1->SET = (1 << 28); // set RESET
  LPC_GPIO1->SET = (1 << 29); // set SCE for falling edge
  // LPC_GPIO1->CLR = (1 << 29); // clear SCE (pg 11)

  for (int i = 0; i < 999; i++) {
  } // waiting for LCD to recognize pins

  fprintf(stderr, "config_lcd_pins() complete\n");
}

void reset_pulse() {
  LPC_GPIO1->SET = (1 << 28);
  LPC_GPIO1->CLR = (1 << 28); // clear RESET, setting it active

  LPC_GPIO1->SET = (1 << 28);

  fprintf(stderr, "reset_pulse() complete\n");
}

void set_display_mode_normal() {
  LPC_GPIO1->CLR = (1 << 30); // set D/C pin to 0, so we can write a command

  // display control function
  // DB3 = 1 == 0000 1XXX
  // DB2 = D (1 for "normal display mode")
  // DB1 = 0
  // DB0 = E (0 for "normal display mode")
  // so bytes should be 0000 1100
  // which is 0x0C in hex

  LPC_GPIO1->CLR = (1 << 29);

  ssp2__exchange_byte(0x0C); // command for "display mode = normal"

  LPC_GPIO1->SET = (1 << 30); // set D/C pin to 1, so normal data can be exchanged

  fprintf(stderr, "set_display_mode_normal() complete\n");
}

void increment_cursor(cursor pos) {
  pos.x++;

  if (pos.x >= 83) {
    pos.x = 0;
    // TODO:
    // increment y
    // send new yPos
  }

  LPC_GPIO1->CLR = (1 << 30); // enabling command transf

  for (int i = 0; i < 500; i++) {
  } // delay so change in D/C pin is recognized by LCD

  // to set X address of cursor (pg 14)
  // DB7 is 1, DB6 thru DB0 are the bits of the X position
  // so value should be:   1 X6 X5 X4   X3 X2 X1 X0

  uint16_t send = 0;
  send |= 0b10000000;
  send |= pos.x;

  ssp2__exchange_byte(send);

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

  ssp2__exchange_byte(send);

  // Sending Y position (pg 14)
  // DB6 should be 1
  // DB2 = Y2, DB1 = Y1, DB0 = Y0, but initially, all Y values are zero

  send = 0b01000000;
  ssp2__exchange_byte(send);
  fprintf(stderr, "Set initial cursor positions\n");
}

void function_set() {
  LPC_GPIO1->CLR = (1 << 30); // enabling command mode
  LPC_GPIO1->CLR = (1 << 29);
  ssp2__exchange_byte(0b00100010); // pg 22 step 2

  // ssp2__exchange_byte(0b10010000); // pg 22 step 3
  // function set is:
  // 0010   0 PD V H, where...
  //      PD = 0 means chip is active, PD=1 means power down mode
  //          we're setting the chip active, so PD = 0
  //      V = 0 means horizontal addressing, V = 1 means vertical addressing
  //          V = 0 (pg 22)
  //      H = 0 means basic instruction set, H = 1 means extended instruction set
  //          we're using the basic instruction set, so H = 0
  // uint8_t send = 0b00100000;

  // fprintf(stderr, "send was %d, should have been 32\n", send);

  // ssp2__exchange_byte(send);

  fprintf(stderr, "function_set() complete\n");
}

void init_lcd(cursor pos) {
  // spi1_init();
  config_lcd_pins();
  reset_pulse(); // step 8.1/8.2 on pg 15
  // function_set();            // step 8.3 on pg 15
  set_display_mode_normal(); // step 8.4 on pg 15
  // send_initial_cursor_positions(pos); // step 8.5 and 8.6

  fprintf(stderr, "LCD Setup Complete!\n");
}

void lcd_setup_from_arduino() {
  ssp2__init(4);
  gpio__construct_as_output(1, 28); // RESET
  gpio__construct_as_output(1, 29); // SCE
  gpio__construct_as_output(1, 30); // DC

  LPC_GPIO1->SET = (1 << 28); // setting RESET
  LPC_GPIO1->SET = (1 << 29); // setting SCE
  LPC_GPIO1->CLR = (1 << 28); // clearing RESET
  delay(100);
  LPC_GPIO1->SET = (1 << 28); // setting RESET

  // set LCD Parameters
  lcd_write_command(0x21);
  lcd_write_command(0x13);
  lcd_write_command(0xC2);

  // extended instruction set control
  lcd_write_command(0x20);

  // all display segments on
  lcd_write_command(0x09);

  // Activate LCD
  lcd_write_command(0x08); // display blank
  lcd_write_command(0x0C); // inverse mode (0x0C for normal mode, 0x0D for inverse mode)
  delay(100);

  // Place the cursor at the origin
  lcd_write_command(0x80);
  lcd_write_command(0x40);
}

void lcd_write_command(uint8_t val) {
  LPC_GPIO1->CLR = (1 << 29); // clearing SCE, since it's active low
  LPC_GPIO1->CLR = (1 << 30); // setting DC to low, for a command input
  ssp2__exchange_byte(val);
}

void lcd_write_data(uint8_t val) {
  LPC_GPIO1->CLR = (1 << 29); // clearing SCE, since it's active low
  LPC_GPIO1->SET = (1 << 30); // setting DC to high, for a data input
  ssp2__exchange_byte(val);
}

void delay(int milliseconds) {
  long pause;
  clock_t now, then;

  pause = milliseconds * (CLOCKS_PER_SEC / 1000);
  now = then = clock();
  while ((now - then) < pause)
    now = clock();
}

void lcd_putchar(char in) {

  in = toupper(in);

  switch (in) {
  case 'A':
    // 0xFC, 0x12, 0x11, 0x12, 0xFC, 0x00,
    lcd_write_data(0xFC);
    lcd_write_data(0x12);
    lcd_write_data(0x11);
    lcd_write_data(0x12);
    lcd_write_data(0xFC);
    lcd_write_data(0x00);
    break;

  case 'B':
    // 0xFE, 0x92, 0x92, 0x92, 0x6C, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x6C);
    lcd_write_data(0x00);
    break;

  case 'C':
    // 0x7C, 0x82, 0x82, 0x82, 0x82, 0x00,
    lcd_write_data(0x7C);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x00);
    break;

  case 'D':
    // 0xFE, 0x82, 0x82, 0x82, 0x7C, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x7C);
    lcd_write_data(0x00);
    break;

  case 'E':
    // 0xFE, 0x92, 0x92, 0x92, 0x82, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x82);
    lcd_write_data(0x00);
    break;

  case 'F':
    // 0xFE, 0x12, 0x12, 0x02, 0x02, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x12);
    lcd_write_data(0x12);
    lcd_write_data(0x02);
    lcd_write_data(0x02);
    lcd_write_data(0x00);
    break;

  case 'G':
    // 0x7C, 0x82, 0x92, 0x92, 0x62, 0x00,
    lcd_write_data(0x7C);
    lcd_write_data(0x82);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x62);
    lcd_write_data(0x00);
    break;

  case 'H':
    // 0xFE, 0x10, 0x10, 0x10, 0xFE, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x10);
    lcd_write_data(0x10);
    lcd_write_data(0x10);
    lcd_write_data(0xFE);
    lcd_write_data(0x00);
    break;

  case 'I':
    // 0x82, 0x82, 0xFE, 0x82, 0x82, 0x00,
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0xFE);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x00);
    break;

  case 'J':
    // 0x42, 0x82, 0x42, 0x3E, 0x02, 0x00,
    lcd_write_data(0x42);
    lcd_write_data(0x82);
    lcd_write_data(0x42);
    lcd_write_data(0x3E);
    lcd_write_data(0x02);
    lcd_write_data(0x00);
    break;

  case 'K':
    // 0xFE, 0x10, 0x28, 0x44, 0x82, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x10);
    lcd_write_data(0x28);
    lcd_write_data(0x44);
    lcd_write_data(0x82);
    lcd_write_data(0x00);
    break;

  case 'L':
    // 0xFE, 0x80, 0x80, 0x80, 0x80, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x80);
    lcd_write_data(0x80);
    lcd_write_data(0x80);
    lcd_write_data(0x80);
    lcd_write_data(0x00);
    break;

  case 'M':
    // 0xFC, 0x08, 0x10, 0x08, 0xFC, 0x00,
    lcd_write_data(0xFC);
    lcd_write_data(0x08);
    lcd_write_data(0x10);
    lcd_write_data(0x08);
    lcd_write_data(0xFC);
    lcd_write_data(0x00);
    break;

  case 'N':
    // 0xFC, 0x08, 0x10, 0x20, 0xFC, 0x00,
    lcd_write_data(0xFC);
    lcd_write_data(0x08);
    lcd_write_data(0x10);
    lcd_write_data(0x20);
    lcd_write_data(0xFC);
    lcd_write_data(0x00);
    break;

  case 'O':
    // 0x7C, 0x82, 0x82, 0x82, 0x7C, 0x00,
    lcd_write_data(0x7C);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x82);
    lcd_write_data(0x7C);
    lcd_write_data(0x00);
    break;

  case 'P':
    // 0xFE, 0x12, 0x12, 0x1E, 0x00, 0x00
    lcd_write_data(0xFE);
    lcd_write_data(0x12);
    lcd_write_data(0x12);
    lcd_write_data(0x1E);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    break;

  case 'Q':
    // 0x7C, 0x82, 0xA2, 0x42, 0xBC, 0x00,
    lcd_write_data(0x7C);
    lcd_write_data(0x82);
    lcd_write_data(0xA2);
    lcd_write_data(0x42);
    lcd_write_data(0xBC);
    lcd_write_data(0x00);
    break;

  case 'R':
    // 0xFE, 0x22, 0x22, 0x62, 0x9C, 0x00,
    lcd_write_data(0xFE);
    lcd_write_data(0x22);
    lcd_write_data(0x22);
    lcd_write_data(0x62);
    lcd_write_data(0x9C);
    lcd_write_data(0x00);
    break;

  case 'S':
    // 0x8C, 0x92, 0x92, 0x92, 0x62, 0x00,
    lcd_write_data(0x8C);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x92);
    lcd_write_data(0x62);
    lcd_write_data(0x00);
    break;

  case 'T':
    // 0x02, 0x02, 0xFE, 0x02, 0x02, 0x00,
    lcd_write_data(0x02);
    lcd_write_data(0x02);
    lcd_write_data(0xFE);
    lcd_write_data(0x02);
    lcd_write_data(0x02);
    lcd_write_data(0x00);
    break;

  case 'U':
    // 0x7E, 0x80, 0x80, 0x80, 0x7E, 0x00,
    lcd_write_data(0x7E);
    lcd_write_data(0x80);
    lcd_write_data(0x80);
    lcd_write_data(0x80);
    lcd_write_data(0x7E);
    lcd_write_data(0x00);
    break;

  case 'V':
    // 0x3E, 0x40, 0x80, 0x40, 0x3E, 0x00,
    lcd_write_data(0x3E);
    lcd_write_data(0x40);
    lcd_write_data(0x80);
    lcd_write_data(0x40);
    lcd_write_data(0x3E);
    lcd_write_data(0x00);
    break;

  case 'W':
    // 0x7E, 0x80, 0x78, 0x80, 0x7E, 0x00,
    lcd_write_data(0x7E);
    lcd_write_data(0x80);
    lcd_write_data(0x78);
    lcd_write_data(0x80);
    lcd_write_data(0x7E);
    lcd_write_data(0x00);
    break;

  case 'X':
    // 0xC6, 0x28, 0x10, 0x28, 0xC6, 0x00,
    lcd_write_data(0xC6);
    lcd_write_data(0x28);
    lcd_write_data(0x10);
    lcd_write_data(0x28);
    lcd_write_data(0xC6);
    lcd_write_data(0x00);
    break;

  case 'Y':
    // 0x0E, 0x10, 0xE0, 0x10, 0x0E, 0x00,
    lcd_write_data(0x0E);
    lcd_write_data(0x10);
    lcd_write_data(0xE0);
    lcd_write_data(0x10);
    lcd_write_data(0x0E);
    lcd_write_data(0x00);
    break;

  case 'Z':
    // 0xC2, 0xA2, 0x92, 0x8A, 0x86, 0x00,
    lcd_write_data(0xC2);
    lcd_write_data(0xA2);
    lcd_write_data(0x92);
    lcd_write_data(0x8A);
    lcd_write_data(0x86);
    lcd_write_data(0x00);
    break;

  case ' ':
    // 0xC2, 0xA2, 0x92, 0x8A, 0x86, 0x00,
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    break;
  }
}

void lcd_putstring(char *in) {
  int elements = strlen(in);
  printf("String: ");
  printf(in);
  printf("Element Count: %d", elements);

  for (int i = 0; i < elements; i++) {
    lcd_putchar(in[i]);
  }
}

#endif
