/**
 * @cond internal
 * @file
 */
#include "i2c.h"
#include "i2c_config.h"

// Apply default configuration values
// Include template for each enabled I2C
// Define i2cX_init()

// Command to duplicate I2CC code for each I2C.
// Vimmers will use it with " :*! ".
//   python -c 'import sys,re; s=sys.stdin.read(); print "\n".join(re.sub("(I2C\x7ci2c\x7c )C", r"\1"+x, s) for x in "CDEF")'

#if (defined I2CC_MASTER) || (defined I2CC_SLAVE)
# define X_(p,s)  p ## C ## s
# ifndef I2CC_BAUDRATE
#  define I2CC_BAUDRATE  I2C_BAUDRATE
# endif
# ifndef I2CC_INTLVL
#  define I2CC_INTLVL  I2C_INTLVL
# endif
# ifdef I2CC_MASTER
#  include "masterx.inc.c"
# else
#  include "slavex.inc.c"
# endif
#else
# define i2cC_init()
#endif

#if (defined I2CD_MASTER) || (defined I2CD_SLAVE)
# define X_(p,s)  p ## D ## s
# ifndef I2CD_BAUDRATE
#  define I2CD_BAUDRATE  I2C_BAUDRATE
# endif
# ifndef I2CD_INTLVL
#  define I2CD_INTLVL  I2C_INTLVL
# endif
# ifdef I2CD_MASTER
#  include "masterx.inc.c"
# else
#  include "slavex.inc.c"
# endif
#else
# define i2cD_init()
#endif

#if (defined I2CE_MASTER) || (defined I2CE_SLAVE)
# define X_(p,s)  p ## E ## s
# ifndef I2CE_BAUDRATE
#  define I2CE_BAUDRATE  I2C_BAUDRATE
# endif
# ifndef I2CE_INTLVL
#  define I2CE_INTLVL  I2C_INTLVL
# endif
# ifdef I2CE_MASTER
#  include "masterx.inc.c"
# else
#  include "slavex.inc.c"
# endif
#else
# define i2cE_init()
#endif

#if (defined I2CF_MASTER) || (defined I2CF_SLAVE)
# define X_(p,s)  p ## F ## s
# ifndef I2CF_BAUDRATE
#  define I2CF_BAUDRATE  I2C_BAUDRATE
# endif
# ifndef I2CF_INTLVL
#  define I2CF_INTLVL  I2C_INTLVL
# endif
# ifdef I2CF_MASTER
#  include "masterx.inc.c"
# else
#  include "slavex.inc.c"
# endif
#else
# define i2cF_init()
#endif



void i2c_init(void)
{
  i2cC_init();
  i2cD_init();
  i2cE_init();
  i2cF_init();
}



int8_t i2cm_send(i2cm_t *m, uint8_t addr, const uint8_t *data, uint8_t n)
{
  uint8_t status;

  // slave address + Write bit (0)
  m->ADDR = addr << 1;
  while(!((status = m->STATUS) & (TWI_MASTER_RIF_bm|TWI_MASTER_WIF_bm))) ;
  if(!(status & TWI_MASTER_WIF_bm)) {
    return -1;
  } else if(status & TWI_MASTER_RXACK_bm) {
    // NACK
    m->CTRLC = TWI_MASTER_CMD_STOP_gc;
    return 0;
  }

  uint8_t i;
  for(i=0; i<n; ) {
    m->DATA = data[i++];
    while(!((status = m->STATUS) & (TWI_MASTER_RIF_bm|TWI_MASTER_WIF_bm))) ;
    if(!(status & TWI_MASTER_WIF_bm)) {
      return -1;
    } else if(status & TWI_MASTER_RXACK_bm) {
      break;
    }
  }
  m->CTRLC = TWI_MASTER_CMD_STOP_gc;

  return i;
}


int8_t i2cm_recv(i2cm_t *m, uint8_t addr, uint8_t *data, uint8_t n)
{
  uint8_t status;

  // slave address + Read bit (1)
  m->ADDR = (addr << 1) + 1;
  while(!((status = m->STATUS) & (TWI_MASTER_RIF_bm|TWI_MASTER_WIF_bm))) ;
  if(status & (TWI_MASTER_ARBLOST_bm|TWI_MASTER_BUSERR_bm)) {
    return -1;
  } else if(!(status & TWI_MASTER_RIF_bm)) {
    // NACK
    m->CTRLC = TWI_MASTER_CMD_STOP_gc;
    return 0;
  }

  uint8_t i;
  for(i=0; i<n-1; i++) {
    data[i] = m->DATA;
    m->CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
    while(!((status = m->STATUS) & TWI_MASTER_RIF_bm)) ;
  }
  data[i++] = m->DATA;
  m->CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;

  return i;
}


//@endcond
