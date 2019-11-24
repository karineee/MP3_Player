// Emil's SSP Driver
#pragma once

#include <stdint.h>
#include <stdlib.h>

// SSP2 is connected to External Flash memory and SD card reader

void ssp2__init(uint32_t max_clock_mhz);

void ssp0__init(uint32_t max_clock_mhz);

uint8_t ssp0__exchange_byte(uint8_t data_out);

uint8_t ssp2L__exchange_byte(uint8_t data_out);

uint16_t ssp0__exchange_half(uint16_t data_out);
