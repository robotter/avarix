/** @defgroup timer Timer
 * @brief Timer module
 *
 * Timers are used to schedule periodic actions.
 * Timing is ensured by a timer/counter (TC module). Events are executed on
 * compare interrupts, allowing one scheduled event per timer channel.
 *
 * @note
 * In this module documentation \e xn is a TC name composed of an
 * upper-case letter and a digit (0 or 1), for instance \c C0.
 * \e n value is called the timer type. It is also used alone.
 */
//@{
/**
 * @file
 * @brief Timer definitions
 */
#ifndef TIMER_H__
#define TIMER_H__

#include <avr/io.h>
#include <stdint.h>
#include <avarix.h>
#include <clock/defs.h>
#include "timer_config.h"


#ifdef DOXYGEN

/// Timer state
typedef struct timer_struct timer_t;

/// State of timerxn
extern timer_t *const timerxn;

# else

// Set macros to apply expression for enabled timers
// Apply default configuration values

// Command to duplicate TIMERC0 code for each timer.
// Vimmers will use it with " :*! ".
//   python -c 'import sys; s=sys.stdin.read(); print "\n".join(s.replace("C0",x+n) for x in "CDEF" for n in "01")'

#ifdef TIMERC0_ENABLED
# define TIMERC0_APPLY_EXPR(f)  f(C0)
# ifndef TIMERC0_PRESCALER_DIV
#  define TIMERC0_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERC0_APPLY_EXPR(f)
#endif

#ifdef TIMERC1_ENABLED
# define TIMERC1_APPLY_EXPR(f)  f(C1)
# ifndef TIMERC1_PRESCALER_DIV
#  define TIMERC1_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERC1_APPLY_EXPR(f)
#endif

#ifdef TIMERD0_ENABLED
# define TIMERD0_APPLY_EXPR(f)  f(D0)
# ifndef TIMERD0_PRESCALER_DIV
#  define TIMERD0_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERD0_APPLY_EXPR(f)
#endif

#ifdef TIMERD1_ENABLED
# define TIMERD1_APPLY_EXPR(f)  f(D1)
# ifndef TIMERD1_PRESCALER_DIV
#  define TIMERD1_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERD1_APPLY_EXPR(f)
#endif

#ifdef TIMERE0_ENABLED
# define TIMERE0_APPLY_EXPR(f)  f(E0)
# ifndef TIMERE0_PRESCALER_DIV
#  define TIMERE0_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERE0_APPLY_EXPR(f)
#endif

#ifdef TIMERE1_ENABLED
# define TIMERE1_APPLY_EXPR(f)  f(E1)
# ifndef TIMERE1_PRESCALER_DIV
#  define TIMERE1_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERE1_APPLY_EXPR(f)
#endif

#ifdef TIMERF0_ENABLED
# define TIMERF0_APPLY_EXPR(f)  f(F0)
# ifndef TIMERF0_PRESCALER_DIV
#  define TIMERF0_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERF0_APPLY_EXPR(f)
#endif

#ifdef TIMERF1_ENABLED
# define TIMERF1_APPLY_EXPR(f)  f(F1)
# ifndef TIMERF1_PRESCALER_DIV
#  define TIMERF1_PRESCALER_DIV  TIMER_PRESCALER_DIV
# endif
#else
# define TIMERF1_APPLY_EXPR(f)
#endif


// Apply a macro to all enabled timers
#define TIMER_ALL_APPLY_EXPR(f)  \
  TIMERC0_APPLY_EXPR(f) \
  TIMERC1_APPLY_EXPR(f) \
  TIMERD0_APPLY_EXPR(f) \
  TIMERD1_APPLY_EXPR(f) \
  TIMERE0_APPLY_EXPR(f) \
  TIMERE1_APPLY_EXPR(f) \
  TIMERF0_APPLY_EXPR(f) \
  TIMERF1_APPLY_EXPR(f) \


// timer0_struct and timer1_struct are compatible
// use timer0 for common type since it is the largest one
typedef struct timer0_struct timer_t;

#define TIMER_EXPR(xn) \
    extern timer_t *const timer##xn;
TIMER_ALL_APPLY_EXPR(TIMER_EXPR)
#undef TIMER_EXPR

#endif

/** @brief Timer channels
 *
 * Timers of type 0 have four channels.
 * Timers of type 1 have only two channels (A and B).
 *
 * For convenience, values are mapped to ASCII letters A to D.
 */
typedef enum {
  TIMER_CHA = 'A',
  TIMER_CHB = 'B',
  TIMER_CHC = 'C',
  TIMER_CHD = 'D',

} timer_channel_t;

/// Timer event callback
typedef void (*timer_callback_t)(void);


/** @brief Initialize all enabled timers
 */
void timer_init(void);


/** @brief Get underlying TC structure
 * @note The returned pointer actually points to a TC1_t structure for timers
 * of type 1. Since TC0_t and TC1_t are compatible this is fine.
 */
TC0_t *timer_get_tc(const timer_t *t);


/** @brief Schedule a periodic callback on a timer channel
 *
 * @param t  timer to use
 * @param ch  timer channel to use
 * @param period  event period in ticks
 * @param intlvl  event priority level (1, 2 or 3)
 * @param cb  callback to execute
 *
 * @note If \e period is 0, the actual period will be 0x10000 (maximum value).
 */
void timer_set_callback(timer_t *t, timer_channel_t ch, uint16_t period, intlvl_t intlvl, timer_callback_t cb);

/// Cancel a scheduled periodic callback on a timer channel
void timer_clear_callback(timer_t *t, timer_channel_t ch);


/// Convert microseconds to timer ticks
#define TIMER_US_TO_TICKS(xn,us)  (((unsigned long)(us) * (CLOCK_PER_FREQ)) / ((TIMER##xn##_PRESCALER_DIV) * 1000000UL))

/// Convert timer ticks to microseconds
#define TIMER_TICKS_TO_US(xn,ticks)  (1000000UL * (ticks) * (TIMER##xn##_PRESCALER_DIV) / (CLOCK_PER_FREQ))


#endif
//@}
