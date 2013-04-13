/**
 * @cond internal
 * @file
 */
#include <stdint.h>
#include <util/atomic.h>
#include <clock/defs.h>
#include "aeat.h"
#include "aeat_config.h"

#if (CLOCK_PER_FREQ / AEAT_SPI_PRESCALER) > 1000000
# error AEAT_SPI_PRESCALER is too low, max AEAT SPI frequency is 1MHz
#endif


void aeat_spi_init(void)
{
  AEAT_SPI.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_2_gc |
#if AEAT_SPI_PRESCALER == 2
      SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm
#elif AEAT_SPI_PRESCALER == 4
      SPI_PRESCALER_DIV4_gc
#elif AEAT_SPI_PRESCALER == 8
      SPI_PRESCALER_DIV16_gc | SPI_CLK2X_bm
#elif AEAT_SPI_PRESCALER == 16
      SPI_PRESCALER_DIV16_gc
#elif AEAT_SPI_PRESCALER == 32
      SPI_PRESCALER_DIV64_gc | SPI_CLK2X_bm
#elif AEAT_SPI_PRESCALER == 64
      SPI_PRESCALER_DIV64_gc
#elif AEAT_SPI_PRESCALER == 128
      SPI_PRESCALER_DIV128_gc
#else
# error Invalid AEAT_SPI_PRESCALER value
#endif
      ;

  portpin_dirset(&PORTPIN_SPI_MOSI(&AEAT_SPI));
  portpin_dirclr(&PORTPIN_SPI_MISO(&AEAT_SPI));
  portpin_dirset(&PORTPIN_SPI_SCK(&AEAT_SPI));
}


void aeat_init(aeat_t *enc, portpin_t cspp)
{
  enc->cspp = cspp;
  enc->capture = 0;
  enc->value = 0;

  portpin_dirset(&cspp);
  portpin_outset(&cspp);
}


/// Receive a single byte from SPI
static inline uint8_t aeat_spi_recv(void)
{
  AEAT_SPI.DATA = 0;
  while(!(AEAT_SPI.STATUS & SPI_IF_bm)) ;
  return AEAT_SPI.DATA;
}


void aeat_update(aeat_t *enc)
{
  // select the SPI slave
	portpin_outclr(&enc->cspp);

  // capture a new value
  uint8_t msb = ~aeat_spi_recv();
  uint8_t lsb = ~aeat_spi_recv();
  uint16_t capture = (((uint16_t)msb << 8) | lsb) >> 3;

  // deselect the SPI slave
	portpin_outset(&enc->cspp);

  // update encoder state
  uint16_t diff = capture - enc->capture;
  enc->capture = capture;
  INTLVL_DISABLE_BLOCK(INTLVL_HI) {
    enc->value += (int16_t)diff;
  }
}


int32_t aeat_get_value(aeat_t *enc)
{
  int32_t ret;
  INTLVL_DISABLE_BLOCK(INTLVL_HI) {
    ret = enc->value;
  }
  return ret;
}


void aeat_set_value(aeat_t *enc, int32_t v)
{
  INTLVL_DISABLE_BLOCK(INTLVL_HI) {
    enc->value = v;
  }
}


//@endcond
