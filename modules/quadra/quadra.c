/**
 * @cond internal
 * @file
 */
#include "quadra.h"


void quadra_init(quadra_t *q, TC1_t *tc, uint8_t evch, portpin_t pp0, portpin_t pp90, uint8_t samples)
{
  // copy parameters to internal structure
  q->tc = tc;

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


uint16_t quadra_get(quadra_t *q)
{
  return q->tc->CNT;
}


void quadra_reset(quadra_t *q, uint16_t value)
{
  q->tc->CNT = value;
}


//@endcond
