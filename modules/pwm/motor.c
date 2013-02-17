/**
 * @cond internal
 * @file
 */
#include <avarix.h>
#include <clock/defs.h>
#include "motor.h"


struct pwm_motor_struct {
  TC0_t *tc;  ///< timer used to generate the PWM signal
  uint8_t channel;  ///< timer channel used for PWM output, from 0 to 3
  PORT_t *signport;  ///< port of sign output, \e NULL if not used
  uint8_t signpin;  ///< pin of sign outpt, from 0 to 7
};


void pwm_motor_init(pwm_motor_t *pwm, TC0_t *tc, char channel, PORT_t *pwmport, uint8_t pwmpin, PORT_t *signport, uint8_t signpin)
{
  // copy parameters to internal structure
  pwm->tc = tc;
  pwm->channel = channel >= 'A' && channel <= 'D' ? channel - 'A' : 0;
  pwm->signport = signport;
  pwm->signpin = signpin > 7 ? signpin : 0;

  // configure output pins
  pwmport->DIRSET = (1 << pwmpin);
  if(signport) {
    signport->DIRSET = (1 << signpin);
  }

  // configure the timer/counter
  (&tc->CCA)[pwm->channel] = 0; // init duty cycle to the minimum
  tc->CTRLB = (tc->CTRLB & ~TC0_WGMODE_gm) | TC_WGMODE_SS_gc | (1 << (TC0_CCAEN_bp+pwm->channel));
  tc->CTRLA = TC_CLKSEL_DIV1_gc;
}


void pwm_motor_set(pwm_motor_t *pwm, int16_t v)
{
  uint16_t abs = MIN(v < 0 ? -v : v, PWM_MOTOR_MAX);
  (&pwm->tc->CCA)[pwm->channel] = (uint32_t)(pwm->tc->PER * abs)/PWM_MOTOR_MAX;
  if(pwm->signport) {
    if(v < 0) {
      pwm->signport->OUTCLR = (1 << pwm->signpin);
    } else {
      pwm->signport->OUTSET = (1 << pwm->signpin);
    }
  }
}


void pwm_motor_set_frequency(pwm_motor_t *pwm, uint16_t freq)
{
  pwm->tc->PER = (CLOCK_PER_FREQ / freq) - 1;
}


//@endcond
