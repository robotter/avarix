/** @defgroup i2c I2C
 * @brief I2C module
 */
//@{
/**
 * @file
 * @brief I2C definitions
 */
#ifndef I2C_H__
#define I2C_H__

#include <avr/io.h>
#include <avarix/intlvl.h>
#include <stdint.h>
#include "i2c_config.h"

#ifdef DOXYGEN

/// I2C master state
typedef struct i2cm_struct i2cm_t;
/// I2C slave state
typedef struct i2cs_struct i2cs_t;

#else

typedef struct TWI_MASTER_struct i2cm_t;

typedef enum {
  I2CS_STATE_NONE,
  I2CS_STATE_READ,
  I2CS_STATE_WRITE,

} i2cs_state_t;

#ifndef I2CS_RECV_BUFFER_SIZE
# define I2CS_RECV_BUFFER_SIZE  32
#endif

#ifndef I2CS_SEND_BUFFER_SIZE
# define I2CS_SEND_BUFFER_SIZE  32
#endif

/** @brief I2C slave master-write frame received
 *
 * @param buffer buffer containing the received bytes
 * @param n number of bytes received from master
 *
 * This function is called when a master-write operation has completed
 */
typedef void (*i2cs_recv_callback_t)(uint8_t *buffer, uint8_t n);

/** @brief I2C slave master-read operation was requested
 *
 * @param buffer buffer to provision
 * @param maxsz maximum number of bytes which can be written to buffer
 * @return number of bytes to send, returning 0 will result in a NACK from slave.
 *
 * This function is called when a master-read operation was requested by master
 * and ask user to provision the buffer which will be sent.
 */
typedef uint8_t (*i2cs_prepare_send_callback_t)(uint8_t *buffer, uint8_t maxsz);

/** @brief I2C slave transaction finished successfully or not
 *
 * This function is called when a STOP condition or any bus error has ended current transaction
 */
typedef void (*i2cs_reset_callback_t)(void);

typedef struct {

  i2cs_state_t state;

  uint8_t recvd_bytes;
  uint8_t recv_buffer[I2CS_RECV_BUFFER_SIZE];

  uint8_t sent_bytes;
  uint8_t bytes_to_send;
  uint8_t send_buffer[I2CS_SEND_BUFFER_SIZE];

  i2cs_recv_callback_t recv_callback;

  i2cs_prepare_send_callback_t prepare_send_callback;

  i2cs_reset_callback_t reset_callback;

} i2cs_t;

// Check for I2C enabled as both master and slave
// Define pointers to internal structures

// Command to duplicate I2CC code for each I2C.
// Vimmers will use it with " :*! ".
//   python -c 'import sys,re; s=sys.stdin.read(); print "\n".join(re.sub("(I2C\x7ci2c\x7cTWI)C", r"\1"+x, s) for x in "CDEF")'

#if (defined I2CC_MASTER) && (defined I2CC_SLAVE)
# error I2CC enabled as both master and slave
#elif (defined I2CC_MASTER)
# define i2cC  (&TWIC.MASTER)
#elif (defined I2CC_SLAVE)
# define X_(p,s)  p ## C ## s
# include "slavex.inc.h"
#endif

#if (defined I2CD_MASTER) && (defined I2CD_SLAVE)
# error I2CD enabled as both master and slave
#elif (defined I2CD_MASTER)
# define i2cD  (&TWID.MASTER)
#elif (defined I2CD_SLAVE)
# define X_(p,s)  p ## D ## s
# include "slavex.inc.h"
#endif

#if (defined I2CE_MASTER) && (defined I2CE_SLAVE)
# error I2CE enabled as both master and slave
#elif (defined I2CE_MASTER)
# define i2cE  (&TWIE.MASTER)
#elif (defined I2CE_SLAVE)
# define X_(p,s)  p ## E ## s
# include "slavex.inc.h"
#endif

#if (defined I2CF_MASTER) && (defined I2CF_SLAVE)
# error I2CF enabled as both master and slave
#elif (defined I2CF_MASTER)
# define i2cF  (&TWIF.MASTER)
#elif (defined I2CF_SLAVE)
# define X_(p,s)  p ## F ## s
# include "slavex.inc.h"
#endif

#endif


/** @brief Initialize all enabled I2Cs
 */
void i2c_init(void);


/** @brief Send a frame to a slave
 *
 * @param  m  I2C master to use
 * @param  addr  slave address
 * @param  n  size to send (max: 127)
 * @param  data  buffer to send
 *
 * @retval -1  error before first byte has been sent
 * @retval  0  NACK has been immediately received
 * @retval  n  size of sent data (even on error / NACK)
 */
int8_t i2cm_send(i2cm_t *m, uint8_t addr, const uint8_t *data, uint8_t n);

/** @brief Receive a frame from a slave
 *
 * @param  m  I2C master to use
 * @param  addr  slave address
 * @param  n  size to read (max: 127)
 * @param  data  buffer for read data
 *
 * @retval -1  error before first byte has been received
 * @retval  0  NACK has been immediately received
 * @retval  n  size of sent data
 */
int8_t i2cm_recv(i2cm_t *m, uint8_t addr, uint8_t *data, uint8_t n);

#endif
//@}
