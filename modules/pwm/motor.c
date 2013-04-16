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
  pwm->vmax = 0;

  // configure output pins
  portpin_dirset(&PORTPIN_OCNX(tc,pwm->channel));

  // configure the timer/counter
  (&tc->CCA)[pwm->channel] = 0; // init duty cycle to the minimum
  tc->CTRLB = (tc->CTRLB & ~TC0_WGMODE_gm) | TC_WGMODE_SS_gc | (1 << (TC0_CCAEN_bp+pwm->channel));
  tc->CTRLA = TC_CLKSEL_OFF_gc;
}


void pwm_motor_set_frequency(pwm_motor_t *pwm, uint32_t freq)
{
  uint8_t ctrla;
  uint32_t per = CLOCK_PER_FREQ / freq;
  if(per < 0x10000) {
    ctrla = TC_CLKSEL_DIV1_gc;
  } else if(per < 2*0x10000) {
    per /= 2;
    ctrla = TC_CLKSEL_DIV2_gc;
  } else if(per < 4*0x10000) {
    per /= 4;
    ctrla = TC_CLKSEL_DIV4_gc;
  } else if(per < 8*0x10000) {
    per /= 8;
    ctrla = TC_CLKSEL_DIV8_gc;
  } else if(per < 64*0x10000) {
    per /= 64;
    ctrla = TC_CLKSEL_DIV64_gc;
  } else if(per < 256*0x10000) {
    per /= 256;
    ctrla = TC_CLKSEL_DIV256_gc;
  } else if(per < 1024*0x10000) {
    per /= 1024;
    ctrla = TC_CLKSEL_DIV1024_gc;
  } else {
    // not supported, should not happen anyway
    per = 0;
    ctrla = TC_CLKSEL_OFF_gc;
  }

  pwm->tc->PER = per;
  pwm->tc->CTRLA = ctrla;;
  pwm->vmin = 0;
  pwm->vmax = per;
}


void pwm_motor_set_range(pwm_motor_t *pwm, uint16_t tmin, uint16_t tmax)
{
  uint8_t shift;
  switch(pwm->tc->CTRLA & TC0_CLKSEL_gm) {
    case TC_CLKSEL_DIV1_gc: shift = 0; break;
    case TC_CLKSEL_DIV2_gc: shift = 1; break;
    case TC_CLKSEL_DIV4_gc: shift = 2; break;
    case TC_CLKSEL_DIV8_gc: shift = 3; break;
    case TC_CLKSEL_DIV64_gc: shift = 6; break;
    case TC_CLKSEL_DIV256_gc: shift = 8; break;
    case TC_CLKSEL_DIV1024_gc: shift = 10; break;
    default: shift = 0;
  }
  uint16_t per = pwm->tc->PER;
  pwm->vmin = MIN((float)(tmin * CLOCK_PER_FREQ) / (1000000UL << shift), per);
  pwm->vmax = CLAMP((float)(tmax * CLOCK_PER_FREQ) / (1000000UL << shift), pwm->vmin, per);
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
