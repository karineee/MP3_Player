/**
 * @file
 * Analog to Digital Converter driver for LPC40xx
 *
 * @note
 * This driver is intentionally simplified, and is meant to be used as a reference.
 *
 * @to use
 * Call adc__initialize
 * call adc__configure_io_pin on available pin/port
 * call adc__enable_burst_mode for burst mode
 *     with burst mode, use adc__get_adc_value_bm
 * without burst mode, use
 *     adc__get_adc_value
 */
#pragma once

#include <stdint.h>

// Only Channel2, Channel4 and Channel5 pins are avaible for use on SJ2 Development board
typedef enum {
  ADC__CHANNEL_2 = 2, // 0, 25
  ADC__CHANNEL_4 = 4, // 1, 30
  ADC__CHANNEL_5 = 5, // 1, 31
} adc_channel_e;

void adc__initialize(void);

/**
 * Reads the given ADC channal and returns its digital value
 * This starts conversion of one channel, and should not be used from multiple tasks
 */
uint16_t adc__get_adc_value(adc_channel_e channel_num);

uint16_t adc__get_adc_value_bm(adc_channel_e channel_num);

void adc__enable_burst_mode(void);

void adc__configure_io_pin(adc_channel_e channel);