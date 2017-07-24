/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2Cs definitions
 */
#ifndef I2CS_H__
#define I2CS_H__

#include <avr/io.h>
#include <avarix/intlvl.h>
#include <stdint.h>
#include "i2c_config.h"

// Transaction status defines
#define I2CS_STATUS_READY 0
#define I2CS_STATUS_BUSY  1

// Transaction result enumeration
typedef enum {
  I2CS_RESULT_UNKNOWN = 0,
  I2CS_RESULT_RECEIVED,
  I2CS_RESULT_TRANSMIT,
  I2CS_RESULT_OK,
  I2CS_RESULT_BUFFER_OVERFLOW,
  I2CS_RESULT_TRANSMIT_COLLISION,
  I2CS_RESULT_BUS_ERROR,
  I2CS_RESULT_FAIL,
  I2CS_RESULT_ABORTED
} i2c_result_t;

/* @brief I2C slave driver struct.
 *
 *  I2C slave struct. Buffers and necessary varibles.
 */
typedef struct {
  void (*process_data) (void); // Pointer to process data function
  uint8_t received_data[I2CS_RECEIVE_BUFFER_SIZE]; // Read data
  uint8_t transmit_data[I2CS_SEND_BUFFER_SIZE]; // Data to write
  uint8_t bytes_received; // Number of bytes received
  uint8_t bytes_transmit; // Number of bytes sent
  uint8_t status; // Status of transaction
  uint8_t result; // Result of transaction
} i2cs_t;


#endif
//@}
