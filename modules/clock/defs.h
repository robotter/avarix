/** @addtogroup clock */
//@{
/** @file
 * @brief Clock configuration definitions
 */
#ifndef CLOCK_DEFS_H__
#define CLOCK_DEFS_H__

#include <avr/io.h>
#include "clock_config.h"

/** @name Clock source values
 */
//@{
/// No source selected (RTC source only)
#define CLOCK_SOURCE_NONE  0
/// Clock source is the internal 2MHz RC oscillator
#define CLOCK_SOURCE_RC2M  1
/// Clock source is the internal 32MHz RC oscillator
#define CLOCK_SOURCE_RC32M  2
/// Clock source is the internal 32768Hz RC oscillator
#define CLOCK_SOURCE_RC32K  3
/// Clock source is an external oscillator
#define CLOCK_SOURCE_XOSC  4
/// Clock source is an external clock
#define CLOCK_SOURCE_EXTCLK  5
/// Clock source is the internal 32kHz ULP (RTC source only)
#define CLOCK_SOURCE_ULP  6
//@}


#if DOXYGEN

/** @brief Clock source, one of the \c CLOCK_SOURCE_* values
 *
 * This value must be defined in configuration.
 */
#define CLOCK_SOURCE

/** @brief Clock source frequency, in Hz
 *
 * Implied if \ref CLOCK_SOURCE is not \ref CLOCK_SOURCE_XOSC.
 */
#define CLOCK_SOURCE_FREQ

/** @brief System clock frequency (\clk{SYS}), in Hz
 *
 * Value is usually deduced from \ref CLOCK_SOURCE_FREQ and/or
 * \ref CLOCK_PLL_FAC.
 *
 * If these two values differs, PLL is implied and
 * a matching \ref CLOCK_PLL_FAC will be defined.
 */
#define CLOCK_SYS_FREQ

/** @brief CPU clock frequency (\clk{CPU}), in Hz
 *
 * This value is equal to \ref CLOCK_PER_FREQ.
 */
#define CLOCK_CPU_FREQ

/** @brief Peripheral clock frequency (\clk{PER}), in Hz
 *
 * This value is equal to \ref CLOCK_CPU_FREQ.
 */
#define CLOCK_PER_FREQ

/** @name Prescalers and peripheral 2x/4x clocks
 *
 * At least two of these values must be defined.
 * Others will be deduced using \ref CLOCK_CPU_FREQ and \ref CLOCK_SYS_FREQ.
 */
//@{

/// Peripheral 2x clock frequency (\clk{PER2}), in Hz
#define CLOCK_PER2_FREQ
/// Peripheral 4x clock frequency (\clk{PER4}), in Hz
#define CLOCK_PER4_FREQ

/// Prescaler A division ratio
#define CLOCK_PRESCALER_A_DIV
/// Prescaler B division ratio
#define CLOCK_PRESCALER_B_DIV
/// Prescaler C division ratio
#define CLOCK_PRESCALER_C_DIV

//@}

/** @brief External oscillator start-up time (256, 1000 or 16000)
 *
 * Only needed if \ref CLOCK_SOURCE value is \ref CLOCK_SOURCE_XOSC.
 * Defaults to 1000.
 */
#define CLOCK_XTAL_STARTUP

/** @brief PLL factor, unset if PLL is not used
 *
 * Value may be implied if \ref CLOCK_SOURCE_FREQ and \ref CLOCK_SYS_FREQ do
 * not match.
 *
 * @note If \ref CLOCK_SOURCE is \ref CLOCK_SOURCE_RC32M, the PLL applies on
 * the source frequency divided by 4.
 */
#define CLOCK_PLL_FAC

/** @name RTC configuration
 */
//@{

/** @brief RTC source
 *
 * Value may be \ref CLOCK_SOURCE_RC32K, \ref CLOCK_SOURCE_XOSC, \ref
 * CLOCK_SOURCE_ULP or \ref CLOCK_SOURCE_NONE.
 *
 * If both \ref CLOCK_SOURCE and \ref CLOCK_RTC_SOURCE use an external clock,
 * their configuration must match.
 *
 * Defaults to \ref CLOCK_SOURCE_NONE.
 */
#define CLOCK_RTC_SOURCE

/** @brief RTC source frequency, in Hz
 *
 * Implied if \ref CLOCK_RTC_SOURCE is not \ref CLOCK_SOURCE_XOSC in which case
 * value must be 1024 or 32768.
 */
#define CLOCK_RTC_SOURCE_FREQ

/** @brief RTC prescaled frequency, in Hz
 *
 * Implied if \ref CLOCK_RTC_PRESCALER_DIV is defined.
 * Defaults to \ref CLOCK_RTC_SOURCE_FREQ (no prescaling).
 */
#define CLOCK_RTC_FREQ

/** @brief RTC prescaling factor
 *
 * Implied if \ref CLOCK_RTC_SOURCE_FREQ is defined.
 * Defaults to 1 (no prescaling).
 */
#define CLOCK_RTC_PRESCALER_DIV

//@}

#else


// Check and compute CLOCK_SOURCE and CLOCK_SOURCE_FREQ
#ifdef CLOCK_SOURCE_FREQ
# if CLOCK_SOURCE == CLOCK_SOURCE_RC2M
#  define CLOCK_SOURCE_FREQ_  2000000U
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32M
#  define CLOCK_SOURCE_FREQ_  32000000U
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32K
#  define CLOCK_SOURCE_FREQ_  32768U
# elif CLOCK_SOURCE == CLOCK_SOURCE_XOSC
#  if CLOCK_SOURCE_FREQ != 32768 && (CLOCK_SOURCE_FREQ < 400000 || CLOCK_SOURCE_FREQ > 16000000)
#   error XOSC frequency must be 32768Hz or between 0.4MHz and 16MHz
#  endif
# elif CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
# else
#  error Invalid CLOCK_SOURCE value
# endif
# if (defined CLOCK_SOURCE_FREQ_) && CLOCK_SOURCE_FREQ_ != CLOCK_SOURCE_FREQ
#  error CLOCK_SOURCE_FREQ and CLOCK_SOURCE mismatch
# endif
#else
# if CLOCK_SOURCE == CLOCK_SOURCE_RC2M
#  define CLOCK_SOURCE_FREQ  2000000U
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32M
#  define CLOCK_SOURCE_FREQ  32000000U
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32K
#  define CLOCK_SOURCE_FREQ  32768U
# elif CLOCK_SOURCE == CLOCK_SOURCE_XOSC || CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
#  error CLOCK_SOURCE_FREQ must be defined for this CLOCK_SOURCE
# else
#  error Invalid CLOCK_SOURCE value
# endif
#endif

// Handle PLL, compute \cpu{SYS}
#if CLOCK_SOURCE == CLOCK_SOURCE_RC32M
# define CLOCK_PLL_SOURCE_FREQ_ (CLOCK_SOURCE_FREQ/4)
#else
# define CLOCK_PLL_SOURCE_FREQ_ CLOCK_SOURCE_FREQ
#endif
#ifdef CLOCK_PLL_FAC
# define CLOCK_SYS_FREQ ((CLOCK_PLL_SOURCE_FREQ_) * (CLOCK_PLL_FAC))
#elif (defined CLOCK_SYS_FREQ)
# if CLOCK_SYS_FREQ != CLOCK_SOURCE_FREQ
#  define CLOCK_PLL_FAC ((CLOCK_SYS_FREQ) / (CLOCK_PLL_SOURCE_FREQ_))
# endif
#else
# define CLOCK_SYS_FREQ CLOCK_SOURCE_FREQ
#endif
// Check PLL ratio: no rounding, valid value
// Check PLL against selected source
#ifdef CLOCK_PLL_FAC
# if CLOCK_SOURCE == CLOCK_SOURCE_RC2M
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32M
# elif CLOCK_SOURCE == CLOCK_SOURCE_XOSC
# elif CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
# else
#  error Invalid CLOCK_SOURCE for PLL
# endif
# if CLOCK_SOURCE_FREQ < 400000
#  error PLL source must be at least 0.4MHz
# elif CLOCK_SYS_FREQ < CLOCK_PLL_SOURCE_FREQ_
#  error PLLed frequency must be larger than source frequency
# elif CLOCK_SYS_FREQ != ((CLOCK_PLL_SOURCE_FREQ_) * (CLOCK_PLL_FAC))
#  error CLOCK_PLL_FAC is not an integer
# endif
# if CLOCK_PLL_FAC < 1 || CLOCK_PLL_FAC > 31
#  error CLOCK_PLL_FAC must be between 1 and 31
# endif
# if CLOCK_SYS_FREQ < 10000000 || CLOCK_SYS_FREQ > 200000000
#  error PLL output frequency must be between 10MHz and 200MHz
# endif
#endif

// Handle XTALK startup
#if CLOCK_SOURCE == CLOCK_SOURCE_XOSC && CLOCK_SOURCE_FREQ != 32768
# ifndef CLOCK_XTAL_STARTUP
#  define CLOCK_XTAL_STARTUP 1000
# elif CLOCK_XTAL_STARTUP == 256
# elif CLOCK_XTAL_STARTUP == 1000
# elif CLOCK_XTAL_STARTUP == 16000
# else
#  error Invalid XTAL startup time, must be 256, 1000 or 16000
# endif
#endif

// Check and compute CLOCK_RTC_SOURCE and CLOCK_RTC_SOURCE_FREQ
#ifndef CLOCK_RTC_SOURCE
# define CLOCK_RTC_SOURCE CLOCK_SOURCE_NONE
#endif
#ifdef CLOCK_RTC_SOURCE_FREQ
# if CLOCK_RTC_SOURCE == CLOCK_SOURCE_NONE
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_RC32K
#  define CLOCK_RTC_SOURCE_FREQ_  32768U
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_XOSC
#  if CLOCK_RTC_SOURCE_FREQ != 1024 && CLOCK_RTC_SOURCE_FREQ != 32768
#   error RTC XOSC frequency must be 1024Hz or 32768Hz
#  endif
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_ULP
#  define CLOCK_RTC_SOURCE_FREQ_  32000U
# else
#  error Invalid CLOCK_RTC_SOURCE value
# endif
# if (defined CLOCK_RTC_SOURCE_FREQ_) && CLOCK_RTC_SOURCE_FREQ_ != CLOCK_RTC_SOURCE_FREQ
#  error CLOCK_RTC_SOURCE_FREQ and CLOCK_RTC_SOURCE mismatch
# endif
#else
# if CLOCK_RTC_SOURCE == CLOCK_SOURCE_NONE
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_RC32K
#  define CLOCK_RTC_SOURCE_FREQ  32768U
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_XOSC
#  error CLOCK_RTC_SOURCE_FREQ must be defined for this CLOCK_RTC_SOURCE
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_ULP
#  define CLOCK_RTC_SOURCE_FREQ_  32000U
# else
#  error Invalid CLOCK_RTC_SOURCE value
# endif
#endif
#if CLOCK_RTC_SOURCE == CLOCK_SOURCE_XOSC
# if CLOCK_SOURCE == CLOCK_SOURCE_XOSC
#  if CLOCK_SOURCE_FREQ != 32768
#   error XOSC configuration for clock and RTC mismatch
#  endif
# elif CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
#  error Incompatible CLOCK_SOURCE and CLOCK_RTC_SOURCE
# endif
#endif

// Check and compute CLOCK_RTC_FREQ and CLOCK_RTC_PRESCALER_DIV
#if CLOCK_RTC_SOURCE != CLOCK_RTC_NONE
# ifdef CLOCK_RTC_PRESCALER_DIV
#  ifndef CLOCK_RTC_FREQ
#   define CLOCK_RTC_FREQ  CLOCK_RTC_SOURCE_FREQ / CLOCK_RTC_PRESCALER_DIV
#  endif
# elif (defined CLOCK_RTC_FREQ)
#   define CLOCK_RTC_PRESCALER_DIV  CLOCK_RTC_SOURCE_FREQ / CLOCK_RTC_FREQ
# else
#  define CLOCK_RTC_PRESCALER_DIV  1
#  define CLOCK_RTC_FREQ  CLOCK_RTC_SOURCE_FREQ
# endif
#endif


// Check or compute \clk{CPU} and \clk{PER}
#if !(defined CLOCK_CPU_FREQ) && (defined CLOCK_PER_FREQ)
# define CLOCK_CPU_FREQ CLOCK_PER_FREQ
#elif !(defined CLOCK_PER_FREQ) && (defined CLOCK_CPU_FREQ)
# define CLOCK_PER_FREQ CLOCK_CPU_FREQ
#elif !(defined CLOCK_CPU_FREQ)
# error Either CLOCK_CPU_FREQ or CLOCK_PER_FREQ must be set
#endif
#if CLOCK_CPU_FREQ != CLOCK_PER_FREQ
# error CLOCK_CPU_FREQ and CLOCK_PER_FREQ must be equal
#endif


// Get prescaler ratios from frequencies when possible
// Some checks are made twice because of newly known values

#ifndef CLOCK_PRESCALER_A_DIV
# if (defined CLOCK_PER4_FREQ)
#  define CLOCK_PRESCALER_A_DIV ((CLOCK_SYS_FREQ) / (CLOCK_PER4_FREQ))
# endif
#else
# if !(defined CLOCK_PER4_FREQ)
#  define CLOCK_PER4_FREQ  ((CLOCK_SYS_FREQ) / (CLOCK_PRESCALER_A_DIV))
# endif
#endif

#ifdef CLOCK_PRESCALER_C_DIV
# if (defined CLOCK_PER2_FREQ)
#  define CLOCK_PRESCALER_C_DIV ((CLOCK_PER2_FREQ) / (CLOCK_CPU_FREQ))
# endif
#else
# if !(defined CLOCK_PER2_FREQ)
#  define CLOCK_PER2_FREQ  ((CLOCK_CPU_FREQ) * (CLOCK_PRESCALER_C_DIV))
# endif
#endif

#ifndef CLOCK_PRESCALER_A_DIV
# if (defined CLOCK_PER4_FREQ)
#  define CLOCK_PRESCALER_A_DIV ((CLOCK_SYS_FREQ) / (CLOCK_PER4_FREQ))
# endif
#endif

#ifndef CLOCK_PRESCALER_B_DIV
# if (defined CLOCK_PER4_FREQ) && (defined CLOCK_PER2_FREQ)
#  define CLOCK_PRESCALER_B_DIV ((CLOCK_PER4_FREQ) / (CLOCK_PER2_FREQ))
# endif
#endif

#ifndef CLOCK_PRESCALER_C_DIV
# if (defined CLOCK_PER2_FREQ)
#  define CLOCK_PRESCALER_C_DIV ((CLOCK_PER2_FREQ) / (CLOCK_CPU_FREQ))
# endif
#endif


// Check prescaler ratios

#ifndef CLOCK_PRESCALER_A_DIV
# error Cannot determine CLOCK_PRESCALER_A_DIV
#elif CLOCK_PRESCALER_A_DIV == 1
#elif CLOCK_PRESCALER_A_DIV == 2
#elif CLOCK_PRESCALER_A_DIV == 4
#elif CLOCK_PRESCALER_A_DIV == 8
#elif CLOCK_PRESCALER_A_DIV == 16
#elif CLOCK_PRESCALER_A_DIV == 32
#elif CLOCK_PRESCALER_A_DIV == 64
#elif CLOCK_PRESCALER_A_DIV == 128
#elif CLOCK_PRESCALER_A_DIV == 256
#elif CLOCK_PRESCALER_A_DIV == 512
#else
# error Invalid CLOCK_PRESCALER_A_DIV value
#endif

#if !(defined CLOCK_PRESCALER_B_DIV)
# error Cannot determine CLOCK_PRESCALER_B_DIV
#elif !(defined CLOCK_PRESCALER_C_DIV)
# error Cannot determine CLOCK_PRESCALER_C_DIV
#elif CLOCK_PRESCALER_B_DIV == 1 && CLOCK_PRESCALER_C_DIV == 1
#elif CLOCK_PRESCALER_B_DIV == 1 && CLOCK_PRESCALER_C_DIV == 2
#elif CLOCK_PRESCALER_B_DIV == 2 && CLOCK_PRESCALER_C_DIV == 2
#elif CLOCK_PRESCALER_B_DIV == 4 && CLOCK_PRESCALER_C_DIV == 1
#else
# error Invalid CLOCK_PRESCALER_B_DIV and CLOCK_PRESCALER_C_DIV values
#endif

#ifndef CLOCK_RTC_PRESCALER_DIV
#elif CLOCK_RTC_PRESCALER_DIV == 1
#elif CLOCK_RTC_PRESCALER_DIV == 2
#elif CLOCK_RTC_PRESCALER_DIV == 8
#elif CLOCK_RTC_PRESCALER_DIV == 16
#elif CLOCK_RTC_PRESCALER_DIV == 64
#elif CLOCK_RTC_PRESCALER_DIV == 256
#elif CLOCK_RTC_PRESCALER_DIV == 1024
#else
# error Invalid CLOCK_RTC_PRESCALER_DIV value
#endif

// Check consistency between ratios and frequencies
// Also detect rounding errors
#if ((CLOCK_CPU_FREQ) * (CLOCK_PRESCALER_C_DIV)) != CLOCK_PER2_FREQ
# error CLOCK_CPU_FREQ, CLOCK_PER2_FREQ and CLOCK_PRESCALER_C_DIV mismatch
#elif ((CLOCK_PER2_FREQ) * (CLOCK_PRESCALER_B_DIV)) != CLOCK_PER4_FREQ
# error CLOCK_PER2_FREQ, CLOCK_PER4_FREQ and CLOCK_PRESCALER_B_DIV mismatch
#elif ((CLOCK_PER4_FREQ) * (CLOCK_PRESCALER_A_DIV)) != CLOCK_SYS_FREQ
# error CLOCK_PER4_FREQ, CLOCK_SYS_FREQ and CLOCK_PRESCALER_A_DIV mismatch
#endif
#ifdef CLOCK_RTC_FREQ
# if ((CLOCK_RTC_FREQ) * (CLOCK_RTC_PRESCALER_DIV)) != CLOCK_RTC_SOURCE_FREQ
#  error CLOCK_RTC_FREQ, CLOCK_RTC_SOURCE_FREQ and CLOCK_RTC_PRESCALER_DIV mismatch
# endif
#endif


#endif

#endif
//@}
