/**
 * @cond internal
 * @file
 */
#include <avarix.h>
#include <clock/defs.h>
#include "motor.h"


void pwm_motor_init(pwm_motor_t *pwm, TC0_t *tc, char channel, pwm_motor_sign_cb set_sign)
{
  // initialize internal structure
  pwm->tc = tc;
  pwm->channel = channel >= 'A' && channel <= 'D' ? channel - 'A' : 0;
  pwm->set_sign = set_sign;
  pwm->vmin = 0;
  pwm->vmax = PWM_MOTOR_MAX;

  // configure output pins
  portpin_dirset(&PORTPIN_OCNX(tc,pwm->channel));

  // configure the timer/counter
  (&tc->CCA)[pwm->channel] = 0; // init duty cycle to the minimum
  tc->CTRLB = (tc->CTRLB & ~TC0_WGMODE_gm) | TC_WGMODE_SS_gc | (1 << (TC0_CCAEN_bp+pwm->channel));
  tc->CTRLA = TC_CLKSEL_DIV1_gc;
}


void pwm_motor_set_frequency(pwm_motor_t *pwm, uint16_t freq)
{
  uint16_t per = (CLOCK_PER_FREQ / freq) - 1;
  pwm->tc->PER = per;
  pwm->vmin = MIN(pwm->vmin, per);
  pwm->vmax = MIN(pwm->vmax, per);
}


void pwm_motor_set_range(pwm_motor_t *pwm, uint16_t tmin, uint16_t tmax)
{
  uint16_t per = pwm->tc->PER;
  pwm->vmin = MIN((float)tmin * CLOCK_PER_FREQ / 1000000, per);
  pwm->vmax = CLAMP((float)tmax * CLOCK_PER_FREQ / 1000000, pwm->vmin, per);
}


void pwm_motor_set(pwm_motor_t *pwm, int16_t v)
{
  uint16_t abs = v < 0 ? -v : v;
  (&pwm->tc->CCA)[pwm->channel] = pwm->vmin + (uint16_t)(((uint32_t)abs*(pwm->vmax-pwm->vmin))/((uint16_t)PWM_MOTOR_MAX+1));
  if(pwm->set_sign) {
    pwm->set_sign(v >= 0);
  }
}


void pwm_servo_init(pwm_motor_t *pwm, TC0_t *tc, char channel)
{
  pwm_motor_init(pwm, tc, channel, 0);
  pwm_motor_set_frequency(pwm, PWM_SERVO_FREQ);
}


//@endcond
