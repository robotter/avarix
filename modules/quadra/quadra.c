/**
 * @cond internal
 * @file
 */
#include "quadra.h"


void quadra_init(quadra_t *q, TC1_t *tc, uint8_t evch, PORT_t *port0, uint8_t pin0, PORT_t *port90, uint8_t pin90, uint8_t samples)
{
  // copy parameters to internal structure
  q->tc = tc;

  // configure input pins
  port0->DIRCLR = (1 << pin0);
  port90->DIRCLR = (1 << pin90);
  (&port0->PIN0CTRL)[pin0] = ((&port0->PIN0CTRL)[pin0] & ~PORT_ISC_gm) | PORT_ISC_LEVEL_gc;
  (&port90->PIN0CTRL)[pin90] = ((&port90->PIN0CTRL)[pin90] & ~PORT_ISC_gm) | PORT_ISC_LEVEL_gc;

  // configure the event channel and the timer/counter
  (&EVSYS.CH0MUX)[evch] = EVSYS_CHMUX_PORTA_PIN0_gc + (port0 - &PORTA) * 8 + pin0;
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
