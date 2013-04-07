/**
 * @cond internal
 * @file
 */

#include <stdint.h>
#include <avarix/intlvl.h>
#include "clock.h"


/** @brief Write a protected I/O register
 * @todo Should be put in a dedicated module or header.
 */
static void ccp_io_write(volatile uint8_t *addr, uint8_t value)
{
  INTLVL_DISABLE_ALL_BLOCK() {
    asm volatile (
        "out  %0, %1\n\t"
        "st   Z, %3\n\t"
        :
        : "i" (&CCP)
        , "r" (CCP_IOREG_gc)
        , "z" ((uint16_t)addr)
        , "r" (value)
        );
  }
}


/// Enable an oscillator and wait for it to be ready
#define CLOCK_OSC_ENABLE_AND_WAIT(type)  do { \
  OSC.CTRL |= OSC_ ## type ## EN_bm; \
  while( !(OSC.STATUS & OSC_ ## type ## RDY_bm) ) ; \
} while(0)


void clock_init(void)
{
  // enable source clock and RTC source clock
#if CLOCK_SOURCE == CLOCK_SOURCE_RC2M
  CLOCK_OSC_ENABLE_AND_WAIT(RC2M);
#endif
#if CLOCK_SOURCE == CLOCK_SOURCE_RC32M
  CLOCK_OSC_ENABLE_AND_WAIT(RC32M);
#endif
#if CLOCK_SOURCE == CLOCK_SOURCE_RC32K || CLOCK_RTC_SOURCE == CLOCK_SOURCE_RC32K
  CLOCK_OSC_ENABLE_AND_WAIT(RC32K);
#endif
#if CLOCK_SOURCE == CLOCK_SOURCE_XOSC || CLOCK_RTC_SOURCE == CLOCK_SOURCE_XOSC
# if CLOCK_RTC_SOURCE == CLOCK_SOURCE_XOSC || CLOCK_SOURCE_FREQ == 32768
  OSC.XOSCCTRL = OSC_X32KLPM_bm | OSC_XOSCSEL_32KHz_gc;
# else
  OSC.XOSCCTRL =
#  if CLOCK_SOURCE_FREQ < 2000000
      OSC_FRQRANGE_04TO2_gc
#  elif CLOCK_SOURCE_FREQ < 9000000
      OSC_FRQRANGE_2TO9_gc
#  elif CLOCK_SOURCE_FREQ < 12000000
      OSC_FRQRANGE_9TO12_gc
#  else
      OSC_FRQRANGE_12TO16_gc
#  endif
      |
#  if CLOCK_XTAL_STARTUP == 256
      OSC_XOSCSEL_XTAL_256CLK_gc
#  elif CLOCK_XTAL_STARTUP == 1000
      OSC_XOSCSEL_XTAL_1KCLK_gc
#  elif CLOCK_XTAL_STARTUP == 16000
      OSC_XOSCSEL_XTAL_16KCLK_gc
#  endif
      ;
# endif
  CLOCK_OSC_ENABLE_AND_WAIT(XOSC);
#endif
#if CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
  OSC.XOSCCTRL = OSC_XOSCSEL_EXTCLK_gc;
  CLOCK_OSC_ENABLE_AND_WAIT(XOSC);
#endif

#ifdef CLOCK_PLL_FAC
  // Configure and enable PLL
  OSC.PLLCTRL = (CLOCK_PLL_FAC << OSC_PLLFAC_gp) |
# if CLOCK_SOURCE == CLOCK_SOURCE_RC2M
    OSC_PLLSRC_RC2M_gc
# elif CLOCK_SOURCE == CLOCK_SOURCE_RC32M
    OSC_PLLSRC_RC32M_gc
# elif CLOCK_SOURCE == CLOCK_SOURCE_XOSC
    OSC_PLLSRC_XOSC_gc
# elif CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
    OSC_PLLSRC_XOSC_gc
# endif
    ;
  CLOCK_OSC_ENABLE_AND_WAIT(PLL);
#endif

  // set prescalers
  CLK.PSCTRL =
#if CLOCK_PRESCALER_A_DIV == 1
      CLK_PSADIV_1_gc
#elif CLOCK_PRESCALER_A_DIV == 2
      CLK_PSADIV_2_gc
#elif CLOCK_PRESCALER_A_DIV == 4
      CLK_PSADIV_4_gc
#elif CLOCK_PRESCALER_A_DIV == 8
      CLK_PSADIV_8_gc
#elif CLOCK_PRESCALER_A_DIV == 16
      CLK_PSADIV_16_gc
#elif CLOCK_PRESCALER_A_DIV == 32
      CLK_PSADIV_32_gc
#elif CLOCK_PRESCALER_A_DIV == 64
      CLK_PSADIV_64_gc
#elif CLOCK_PRESCALER_A_DIV == 128
      CLK_PSADIV_128_gc
#elif CLOCK_PRESCALER_A_DIV == 256
      CLK_PSADIV_256_gc
#elif CLOCK_PRESCALER_A_DIV == 512
      CLK_PSADIV_512_gc
#endif
      |
#if CLOCK_PRESCALER_B_DIV == 1 && CLOCK_PRESCALER_C_DIV == 1
      CLK_PSBCDIV_1_1_gc
#elif CLOCK_PRESCALER_B_DIV == 1 && CLOCK_PRESCALER_C_DIV == 2
      CLK_PSBCDIV_1_2_gc
#elif CLOCK_PRESCALER_B_DIV == 2 && CLOCK_PRESCALER_C_DIV == 2
      CLK_PSBCDIV_4_1_gc
#elif CLOCK_PRESCALER_B_DIV == 4 && CLOCK_PRESCALER_C_DIV == 1
      CLK_PSBCDIV_2_2_gc
#endif
      ;

  // change clock source, loop until source stabilization
#ifdef CLOCK_PLL_FAC
# define clksel  CLK_SCLKSEL_PLL_gc
#elif CLOCK_SOURCE == CLOCK_SOURCE_RC2M
# define clksel  CLK_SCLKSEL_RC2M_gc
#elif CLOCK_SOURCE == CLOCK_SOURCE_RC32M
# define clksel  CLK_SCLKSEL_RC32M_gc
#elif CLOCK_SOURCE == CLOCK_SOURCE_RC32K
# define clksel  CLK_SCLKSEL_RC32K_gc
#elif CLOCK_SOURCE == CLOCK_SOURCE_XOSC
# define clksel  CLK_SCLKSEL_XOSC_gc
#elif CLOCK_SOURCE == CLOCK_SOURCE_EXTCLK
# define clksel  CLK_SCLKSEL_XOSC_gc
#endif
  do {
    ccp_io_write(&CLK.CTRL, clksel);
  } while( (CLK.CTRL & CLK_SCLKSEL_gm) != clksel );
#undef clksel

  // set RTC clock source and RTC prescaler
#if CLOCK_RTC_SOURCE != CLOCK_SOURCE_NONE
  CLK.RTCCTRL = CLK_RTCEN_bm |
# if CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_RC32K
    CLK_RTCSRC_RCOSC_gc
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_XOSC && CLOCK_RTC_FREQ == 1024
    CLK_RTCSRC_TOSC_gc
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_XOSC && CLOCK_RTC_FREQ == 32768
    CLK_RTCSRC_TOSC32_gc
# elif CLOCK_RTC_SOURCE == CLOCK_RTC_SOURCE_ULP
    CLK_RTCSRC_ULP_gc
# endif
      ;
  RTC.CTRL =
#if CLOCK_RTC_PRESCALER_DIV == 1
    RTC_PRESCALER_DIV1_gc
#elif CLOCK_RTC_PRESCALER_DIV == 2
    RTC_PRESCALER_DIV2_gc
#elif CLOCK_RTC_PRESCALER_DIV == 8
    RTC_PRESCALER_DIV8_gc
#elif CLOCK_RTC_PRESCALER_DIV == 16
    RTC_PRESCALER_DIV16_gc
#elif CLOCK_RTC_PRESCALER_DIV == 64
    RTC_PRESCALER_DIV64_gc
#elif CLOCK_RTC_PRESCALER_DIV == 256
    RTC_PRESCALER_DIV256_gc
#elif CLOCK_RTC_PRESCALER_DIV == 1024
    RTC_PRESCALER_DIV1024_gc
#endif
    ;
#endif

}


//@endcond
