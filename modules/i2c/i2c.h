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


#ifdef DOXYGEN

/// I2C master state
typedef struct i2cm_struct i2cm_t;
/// I2C slave state
typedef struct i2cs_struct i2cs_t;

/// State of i2cx
extern i2cT_t *const i2cx;

#else

typedef struct TWI_MASTER_struct i2cm_t;
typedef struct i2cs_struct i2cs_t;

// Check for I2C enabled as both master and slave
// Define pointers to internal structures

// Command to duplicate I2CC code for each I2C.
// Vimmers will use it with " :*! ".
//   python -c 'import sys,re; s=sys.stdin.read(); print "\n".join(re.sub("(I2C\x7ci2c\x7cTWI)C", r"\1"+x, s) for x in "CDEF")'

#if (defined I2CC_MASTER) && (defined I2CC_SLAVE)
# error I2CC enabled as both master and slave
#elif (defined I2CC_MASTER)
# define i2cC  (&TWIC.MASTER)
#else
extern i2cs_t *const i2cC;
#endif

#if (defined I2CD_MASTER) && (defined I2CD_SLAVE)
# error I2CD enabled as both master and slave
#elif (defined I2CD_MASTER)
# define i2cD  (&TWID.MASTER)
#else
extern i2cs_t *const i2cD;
#endif

#if (defined I2CE_MASTER) && (defined I2CE_SLAVE)
# error I2CE enabled as both master and slave
#elif (defined I2CE_MASTER)
# define i2cE  (&TWIE.MASTER)
#else
extern i2cs_t *const i2cE;
#endif

#if (defined I2CF_MASTER) && (defined I2CF_SLAVE)
# error I2CF enabled as both master and slave
#elif (defined I2CF_MASTER)
# define i2cF  (&TWIF.MASTER)
#else
extern i2cs_t *const i2cF;
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
