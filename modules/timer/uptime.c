#include "timer.h"
// Don't attempt to define anything if uptime is not configured
#ifdef UPTIME_TIMER

#include <avarix/intlvl.h>
#include "uptime.h"


// Check uptime tick period for rounding errors
TIMER_CHECK_US_TO_TICKS_PRECISION(UPTIME_TIMER, UPTIME_TICK_US);


/// Current uptime counter
static uint32_t uptime_val;

/// Interruption routine to update uptime
static void uptime_update(void)
{
  uptime_val += UPTIME_TICK_US;
}


uint32_t uptime_us(void)
{
  uint32_t ret;
  INTLVL_DISABLE_ALL_BLOCK() {
    ret = uptime_val;
  }
  return ret;
}


void uptime_init(void)
{
  uptime_val = 0;
  TIMER_SET_CALLBACK_US(UPTIME_TIMER, UPTIME_TIMER_CHANNEL,
                        UPTIME_TICK_US, INTLVL_HI, uptime_update);
}


#endif
