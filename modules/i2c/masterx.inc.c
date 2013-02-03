/**
 * @internal
 * @file
 * @brief Include template code for i2c.c
 *
 * The X_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <clock/defs.h>

#define I2CX(s) X_(I2C,s)
#define i2cX(s) X_(i2c,s)
#define TWIX X_(TWI,)


// check configuration
#if I2CX(_BAUDRATE) > 400000
# error I2C baudrate must not be higher than 400kHz
#endif

void i2cX(_init)(void)
{
  // write baudrate first (master should be disabled)
  TWIX.MASTER.BAUD = ((CLOCK_SYS_FREQ)/(2*(I2CX(_BAUDRATE)))) - 5;

  TWIX.MASTER.CTRLA =
      TWI_MASTER_ENABLE_bm |
      ((I2CX(_INTLVL) << TWI_MASTER_INTLVL_gp) & TWI_MASTER_INTLVL_gm)
      ;
  TWIX.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}


#undef I2CX
#undef i2cX
#undef TWIX
#undef X_
