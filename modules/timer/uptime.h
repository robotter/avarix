/** @addtogroup timer */
//@{
/** @file
 * @brief Retrieving and using uptime
 */
/** @name Uptime
 *
 * Uptime elements allow to count time since reset (or more exactly, since \ref
 * uptime_init() was called). It is the base mechanics for time-based actions.
 */
//@{
#ifndef TIMER_UPTIME_H__
#define TIMER_UPTIME_H__

#include <stdint.h>
#include "timer.h"

#ifndef UPTIME_TIMER
# error UPTIME_TIMER must be defined to use uptime features
#endif


/// Initialize and start uptime counter
void uptime_init(void);

/// Get current uptime in microseconds
uint32_t uptime_us(void);

#endif
//@}
//@}
