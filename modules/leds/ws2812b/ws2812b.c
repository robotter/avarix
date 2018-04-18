
#include "ws2812b.h"
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <avarix/portpin.h>


void ws2812b_init(ws2812b_t *ws, TC0_t *tc, char channel) {

  // setup internal structure
  ws->tc = tc;
  ws->channel = channel >= 'A' && channel <= 'D' ? channel - 'A': 0;

  // configure output pins
  portpin_dirset(&PORTPIN_OCNX(tc, pwm->channel));

}
