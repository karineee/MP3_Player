#pragma once
#include <stdbool.h>
#include <stdint.h>

//VOL buttons pin configuration:
    //CLK: Clock signal output (to be set as input)
    //DT: The data from turning the knob (to be set as input)
    //SW: The switch button configuration (to be set as input)

void adc__enable_burst_mode() {
  // will set the relevant bits in Control Register (CR) for doing burst mode.
  fprintf(stderr, "2. Initialize + burst mode\n");

  // Datasheet step 1. Power
  // Note: Clear the PDN bit in the AD0CR before clearing this bit, and set this bit before
  // attempting to set PDN.

  LPC_SC->PCONP |= (1 << 12);

  // Step 2. Peripheral clock
  LPC_SC->PCLKSEL |= (1 << 0);

  // Step 3. CR Register for Burst mode
  LPC_ADC->CR |= (1 << 16);

  // Type A IOCON ADC[5] set as an input to receive values from volume knob
  LPC_IOCON->P1_31 |= (1 << 3);
}

void pin_configure_adc_channel_as_io_pin() {
  fprintf(stderr, "Configuring adc channel as IO pin. ");
  gpio__construct_with_function(1, 31, GPIO__FUNCTION_3); // ADC[5]: GPIO Function 3 for adc channel 5
}

void adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_5) {
  uint16_t result = 0;
  //Receive value from Channel 5
  result = adc__get_adc_value(ADC__CHANNEL_5);
  fprintf(stderr, "ADC:Value is ... %i\n", result);
}