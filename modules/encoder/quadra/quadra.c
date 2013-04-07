/**
 * @cond internal
 * @file
 */
#include "quadra.h"


void quadra_init(quadra_t *enc, TC1_t *tc, uint8_t evch, portpin_t pp0, portpin_t pp90, uint8_t samples)
{
  // copy parameters to internal structure
  enc->tc = tc;
  enc->capture = 0;
  enc->value = 0;

  // configure input pins
  portpin_dirclr(&pp0);
  portpin_dirclr(&pp90);
  PORTPIN_CTRL(&pp0) = (PORTPIN_CTRL(&pp0) & ~PORT_ISC_gm) | PORT_ISC_LEVEL_gc;
  PORTPIN_CTRL(&pp90) = (PORTPIN_CTRL(&pp90) & ~PORT_ISC_gm) | PORT_ISC_LEVEL_gc;

  // configure the event channel and the timer/counter
  (&EVSYS.CH0MUX)[evch] = PORTPIN_EVSYS_CHMUX(&pp0);
  (&EVSYS.CH0CTRL)[evch] = EVSYS_QDEN_bm | (samples - 1);
  tc->CTRLD = TC_EVACT_QDEC_gc | (TC_CLKSEL_EVCH0_gc + evch);
  tc->PER = 0xFFFF;
  tc->CTRLA = TC_CLKSEL_DIV1_gc;
}


void quadra_update(quadra_t *enc)
{
  // capture a new value
  uint16_t capture = enc->tc->CNT;

  // update encoder state
  uint16_t diff = capture - enc->capture;
  enc->capture = capture;
  INTLVL_DISABLE_BLOCK(INTLVL_HI) {
    enc->value += (int16_t)diff;
  }
}


int32_t quadra_get_value(quadra_t *enc)
{
  int32_t ret;
  INTLVL_DISABLE_BLOCK(INTLVL_HI) {
    ret = enc->value;
  }
  return ret;
}


void quadra_set_value(quadra_t *enc, int32_t v)
{
  enc->value = v;
}


//@endcond
