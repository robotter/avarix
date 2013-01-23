/**
 * @internal
 * @file
 * @brief Include template code for timer.c
 *
 * The XN_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <util/atomic.h>
#include <clock/defs.h>

#define TCXN(s) XN_(TC,s)
#define timerXN(s) XN_(timer,s)
#define timerXN_ XN_(timer,_)


#if TCXN(_CCC_vect_num)
static timer0_t timerXN_ = { .tc = &TCXN() };
#else
static timer1_t timerXN_ = { .tc = &TCXN() };
#endif
timer_t *const timerXN() = (timer_t*)&timerXN_;


#define PRESCALER_DIV XN_(TIMER,_PRESCALER_DIV)

void timerXN(_init)(void)
{
  TCXN().CTRLA =
#if PRESCALER_DIV == 1
      1
#elif PRESCALER_DIV == 2
      2
#elif PRESCALER_DIV == 4
      3
#elif PRESCALER_DIV == 8
      4
#elif PRESCALER_DIV == 64
      5
#elif PRESCALER_DIV == 256
      6
#elif PRESCALER_DIV == 1024
      7
#else
#error invalid TIMERXN_PRESCALER_DIV value
#endif
      ;
}

#undef PRESCALER_DIV


#define timerXN_event_X(x)  timerXN_.events[TIMER_CH##x-TIMER_CHA]

/// Interrupt handler for channel callback
#define TIMER_ISR_CCX(x) \
  ISR(TCXN(_CC##x##_vect)) \
  { \
    timer_callback_t cb; \
    ATOMIC_BLOCK(ATOMIC_FORCEON) { \
      TCXN().CC##x += timerXN_event_X(x).period; \
      cb = timerXN_event_X(x).callback; \
    } \
    if(cb) { \
      cb(); \
    } \
  }

TIMER_ISR_CCX(A)
TIMER_ISR_CCX(B)
#if TCXN(_CCC_vect_num)
TIMER_ISR_CCX(C)
TIMER_ISR_CCX(D)
#endif

#undef TIMER_ISR_CCX
#undef timerXN_event_X


#undef TCXN
#undef timerXN
#undef timerXN_
#undef XN_
