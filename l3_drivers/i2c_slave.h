#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  I2C__0s,
  I2C__1s,
  I2C__2s,
} i2c_e_slave;

/**
 * I2C peripheral must be initialized before it is used
 * @param peripheral_clock_hz
 * This is speed of the peripheral clock feeding the I2C peripheral; it is used to set the desired_i2c_bus_speed_in_hz
 */

/**
 * Public Functions
 */
void i2c__initialize_slave(i2c_e_slave i2c_number, uint32_t desired_i2c_bus_speed_in_hz, uint32_t peripheral_clock_hz,
                           uint8_t desired_slave_address, volatile uint8_t *memory_ptr);

/**
 * Private Functions
 */
/*
bool i2c__slave_send_data_to_master(uint8_t memory_index, volatile uint32_t *memory);

bool i2c__slave_receive_data_from_master(uint8_t memory_index, uint8_t memory_value);
*/
