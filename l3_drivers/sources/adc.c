#include <stdint.h>

#include "adc.h"
#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void adc__initialize(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__ADC);

  const uint32_t enable_adc_mask = (1 << 21);
  LPC_ADC->CR = enable_adc_mask;

  const uint32_t max_adc_clock = (12 * 1000UL * 1000UL); // 12.4Mhz : max ADC clock in datasheet for lpc40xx
  const uint32_t adc_clock = clock__get_peripheral_clock_hz();

  // APB clock divicer to support max ADC clock
  for (uint32_t divider = 2; divider < 255; divider += 2) {
    if ((adc_clock / divider) < max_adc_clock) {
      LPC_ADC->CR |= (divider << 8);
      break;
    }
  }
}

uint16_t adc__get_adc_value(adc_channel_e channel_num) {
  uint16_t result = 0;
  const uint16_t twelve_bits = 0x0FFF;
  const uint32_t channel_masks = 0xFF;
  const uint32_t start_conversion = (1 << 24);
  const uint32_t start_conversion_mask = (7 << 24); // 3bits - B26:B25:B24
  const uint32_t adc_conversion_complete = (1 << 31);

  if ((ADC__CHANNEL_2 == channel_num) || (ADC__CHANNEL_4 == channel_num) || (ADC__CHANNEL_5 == channel_num)) {
    LPC_ADC->CR &= ~(channel_masks | start_conversion_mask);
    // Set the channel number and start the conversion now
    LPC_ADC->CR |= (1 << channel_num) | start_conversion;

    while (!(LPC_ADC->GDR & adc_conversion_complete)) { // Wait till conversion is complete
      ;
    }
    result = (LPC_ADC->GDR >> 4) & twelve_bits; // 12bits - B15:B4
  }

  return result;
}

uint16_t adc__get_adc_value_bm(adc_channel_e channel_num) {
  uint16_t result = 0;

  switch (channel_num) {
  case ADC__CHANNEL_2:
    result = (LPC_ADC->DR[2] >> 4);
    break;
  case ADC__CHANNEL_4:
    result = (LPC_ADC->DR[4] >> 4);
    break;
  case ADC__CHANNEL_5:
    result = (LPC_ADC->DR[5] >> 4);
    break;
  default:
    result = 0;
    break;
  }
  result = result & 0xFFF;
  return result;
}

void adc__enable_burst_mode(void) {
  LPC_ADC->CR &= ~(7 << 24); // start to 0
  LPC_ADC->CR |= (1 << 16);  // in control register, set burst mode to 1,
}

void adc__configure_io_pin(adc_channel_e channel) {
  // adc0 channel 2 = 0, 25
  // adc0 channel 4 = 1, 30
  // adc0 channel 5 = 1, 31

  switch (channel) {
  case ADC__CHANNEL_2:
    LPC_IOCON->P0_25 |= (1 << 0); // set to adc function

    LPC_IOCON->P0_25 &= ~(1 << 7); // reset to analog mode
    LPC_IOCON->P0_25 &= ~(3 << 3); // disable internal resistors
    LPC_ADC->CR |= (1 << 2);       // ADC control register, set to activate channel
    break;
  case ADC__CHANNEL_4:
    LPC_IOCON->P1_30 &= ~(7 << 0);
    LPC_IOCON->P1_30 |= (3 << 0);

    LPC_IOCON->P1_30 &= ~(1 << 7);
    LPC_IOCON->P1_30 &= ~(3 << 3);
    LPC_ADC->CR |= (1 << 4);
    break;
  case ADC__CHANNEL_5:
    LPC_IOCON->P1_31 &= ~(7 << 0);
    LPC_IOCON->P1_31 |= (3 << 0);

    LPC_IOCON->P1_31 &= ~(1 << 7);
    LPC_IOCON->P1_31 &= ~(3 << 3);
    LPC_ADC->CR |= (1 << 5);
    break;
  default:
    // invalid channel, do nothing!
    break;
  }
}