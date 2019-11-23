#include "i2c_slave.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "common_macros.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdbool.h>

typedef struct {
  LPC_I2C_TypeDef *const registers; ///< LPC memory mapped registers for the I2C bus

  volatile uint8_t *memory;
  volatile uint32_t index;
  bool received_index;
} i2c_slave_s;

static i2c_slave_s i2c_structs[] = {
    {LPC_I2C0},
    {LPC_I2C1},
    {LPC_I2C2},
};

static void i2c__handle_state_machine_slave(i2c_slave_s *i2c);

static void i2c__handle_interrupt(i2c_slave_s *i2c);

static void i2c0s_isr(void) { i2c__handle_interrupt(&i2c_structs[I2C__0s]); }
static void i2c1s_isr(void) { i2c__handle_interrupt(&i2c_structs[I2C__1s]); }
static void i2c2s_isr(void) { i2c__handle_interrupt(&i2c_structs[I2C__2s]); }

void i2c__initialize_slave(i2c_e_slave i2c_number, uint32_t desired_i2c_bus_speed_in_hz, uint32_t peripheral_clock_hz,
                           uint8_t desired_slave_address, volatile uint8_t *memory_ptr) {

  const function__void_f isrs[] = {i2c0s_isr, i2c1s_isr, i2c2s_isr};
  const lpc_peripheral_e peripheral_ids[] = {LPC_PERIPHERAL__I2C0, LPC_PERIPHERAL__I2C1, LPC_PERIPHERAL__I2C2};

  // Make sure our i2c structs size matches our map of ISRs and I2C peripheral IDs
  COMPILE_TIME_ASSERT(ARRAY_SIZE(i2c_structs) == ARRAY_SIZE(isrs));
  COMPILE_TIME_ASSERT(ARRAY_SIZE(i2c_structs) == ARRAY_SIZE(peripheral_ids));

  i2c_slave_s *i2c = &i2c_structs[i2c_number];
  i2c->memory = memory_ptr;
  LPC_I2C_TypeDef *lpc_i2c = i2c->registers;
  const lpc_peripheral_e peripheral_id = peripheral_ids[i2c_number];

  lpc_peripheral__turn_on_power_to(peripheral_id);
  lpc_i2c->CONCLR = 0x6C; // Clear ALL I2C Flags

  /**
   * Per I2C high speed mode:
   * HS mode master devices generate a serial clock signal with a HIGH to LOW ratio of 1 to 2.
   * So to be able to optimize speed, we use different duty cycle for high/low
   *
   * Compute the I2C clock dividers.
   * The LOW period can be longer than the HIGH period because the rise time
   * of SDA/SCL is an RC curve, whereas the fall time is a sharper curve.
   */
  const uint32_t percent_high = 40;
  const uint32_t percent_low = (100 - percent_high);
  const uint32_t max_speed_hz = UINT32_C(1) * 1000 * 1000;
  const uint32_t ideal_speed_hz = UINT32_C(100) * 1000;
  const uint32_t freq_hz = (desired_i2c_bus_speed_in_hz > max_speed_hz) ? ideal_speed_hz : desired_i2c_bus_speed_in_hz;
  const uint32_t half_clock_divider = (peripheral_clock_hz / freq_hz) / 2;

  // Each clock phase contributes to 50%
  lpc_i2c->SCLH = ((half_clock_divider * percent_high) / 100) / 2;
  lpc_i2c->SCLL = ((half_clock_divider * percent_low) / 100) / 2;

  // Set I2C slave address to desired slave address and enable I2C
  lpc_i2c->ADR0 = lpc_i2c->ADR1 = lpc_i2c->ADR2 = lpc_i2c->ADR3 = desired_slave_address;

  // Enable I2C and the interrupt for it
  lpc_i2c->CONSET = 0x44;
  lpc_peripheral__enable_interrupt(peripheral_id, isrs[i2c_number]);
}

bool i2c__slave_send_data_to_master(i2c_slave_s *i2c, volatile uint32_t memory_index, volatile uint32_t *memory) {

  if (memory_index < 256) {
    *memory = *(i2c->memory + memory_index);
    return true;
  }
  // memory index invalid, don't save, return false
  else
    return false;
}

bool i2c__slave_receive_data_from_master(i2c_slave_s *i2c, volatile uint32_t memory_index, uint8_t memory_value) {

  if (memory_index < 256) {
    *(i2c->memory + memory_index) = memory_value;
    return true;
  }

  else
    return false;
}

/**
 * CONSET and CONCLR register bits
 * 0x04 AA
 * 0x08 SI
 * 0x10 STOP
 * 0x20 START
 * 0x40 ENABLE
 */
static void i2c__clear_si_flag_for_hw_to_take_next_action(LPC_I2C_TypeDef *i2c) { i2c->CONCLR = 0x08; }
static void i2c__set_ack_flag(LPC_I2C_TypeDef *i2c) { i2c->CONSET = 0x04; }
static void i2c__set_nack_flag(LPC_I2C_TypeDef *i2c) { i2c->CONCLR = 0x04; }

static void i2c__handle_state_machine_slave(i2c_slave_s *i2c) {
  enum {

    // Slave Receive States (SR):
    I2C__STATE_S_OWN_ADDR_W_ACK = 0x60,
    I2C__STATE_S_MASTER_DATA_ACK = 0x80,
    I2C__STATE_S_MASTER_DATA_NACK = 0x88,
    I2C__STATE_S_STOP_REPEAT = 0xA0,

    // Slave Transmit States (ST):
    /* Also Uses Slave Receive States*/

    I2C__STATE_S_OWN_ADDR_R_ACK = 0xA8,
    I2C__STATE_S_DATA_SENT_ACK = 0xB8,
    I2C__STATE_S_DATA_SENT_NACK = 0xC0,
    I2C__STATE_S_LAST_DATA_SENT_ACK = 0xC8,

  };

  /*
   ***********************************************************************************************************
   * Write-mode state transition :
   * I2C__STATE_START --> I2C__STATE_MT_SLAVE_ADDR_ACK --> I2C__STATE_MT_SLAVE_DATA_ACK -->
   *                                                  ... (I2C__STATE_MT_SLAVE_DATA_ACK) --> (stop)
   *
   * Read-mode state transition :
   * I2C__STATE_START --> I2C__STATE_MT_SLAVE_ADDR_ACK --> dataAcked --> I2C__STATE_REPEAT_START -->
   * I2C__STATE_MR_SLAVE_READ_ACK For 2+ bytes:  I2C__STATE_MR_SLAVE_ACK_SENT --> ... (I2C__STATE_MR_SLAVE_ACK_SENT) -->
   * I2C__STATE_MR_SLAVE_NACK_SENT --> (stop) For 1  byte :  I2C__STATE_MR_SLAVE_NACK_SENT --> (stop)
   ***********************************************************************************************************
   */

  LPC_I2C_TypeDef *lpc_i2c = i2c->registers;
  const unsigned i2c_state = lpc_i2c->STAT;
  switch (i2c_state) {

  case I2C__STATE_S_OWN_ADDR_W_ACK:
    // Slave is now addresed, will receive base register address
    i2c->received_index = false;
    i2c__set_ack_flag(lpc_i2c);

    i2c__clear_si_flag_for_hw_to_take_next_action(lpc_i2c);
    break;

  case I2C__STATE_S_MASTER_DATA_ACK:
    // receiving data
    if (!(i2c->received_index)) {
      i2c->index = lpc_i2c->DAT;
      i2c->received_index = true;
    } else {

      if (i2c__slave_receive_data_from_master(i2c, i2c->index, lpc_i2c->DAT)) {
        i2c->index = i2c->index + 1;
        i2c__set_ack_flag(lpc_i2c);
      } else {
        i2c__set_nack_flag(lpc_i2c);
      }
    }
    i2c__clear_si_flag_for_hw_to_take_next_action(lpc_i2c);
    break;

  case I2C__STATE_S_OWN_ADDR_R_ACK:

    if (i2c__slave_send_data_to_master(i2c, i2c->index, &lpc_i2c->DAT)) {
      i2c->index = i2c->index + 1;
      i2c__set_ack_flag(lpc_i2c);
    }
    if (i2c->index == 255) {
      i2c__set_nack_flag(lpc_i2c);
    }
    i2c__clear_si_flag_for_hw_to_take_next_action(lpc_i2c);
    break;

  case I2C__STATE_S_DATA_SENT_ACK:

    i2c__slave_send_data_to_master(i2c, i2c->index, &lpc_i2c->DAT);
    i2c->index = i2c->index + 1;
    if (i2c->index == 255) { // next byte shall be the last byte to send
      i2c__set_nack_flag(lpc_i2c);
    } else { // continue sending bytes;
      i2c__set_ack_flag(lpc_i2c);
    }
    i2c__clear_si_flag_for_hw_to_take_next_action(lpc_i2c);
    break;

  case I2C__STATE_S_DATA_SENT_NACK:
  case I2C__STATE_S_LAST_DATA_SENT_ACK:
  case I2C__STATE_S_MASTER_DATA_NACK:
  case I2C__STATE_S_STOP_REPEAT:
  default:
    i2c__set_ack_flag(lpc_i2c);
    i2c__clear_si_flag_for_hw_to_take_next_action(lpc_i2c);
    break;
  }
}

static void i2c__handle_interrupt(i2c_slave_s *i2c) { i2c__handle_state_machine_slave(i2c); }
