/**
 * @file
 * @brief Tools to work with registers
 */
//@{
#ifndef AVARIX_REGISTER_H__
#define AVARIX_REGISTER_H__

#include <avr/io.h>


/** @brief Set a register with I/O CCP disabled
 *
 * Interrupts are not disabled during the process.
 *
 * @note This can be achieved in less cycles for some I/O registers but this way
 * is generic.
 */
static inline void ccp_io_write(volatile uint8_t* addr, uint8_t value)
{
  asm volatile (
      "out  %[ccp], %[ccp_ioreg]\n"
      "st   Z, %[val]\n"
      :
      : [ccp]       "i" (&CCP)
      , [ccp_ioreg] "r" (CCP_IOREG_gc)
      ,             "z" ((uint16_t)addr)
      , [val]       "r" (value)
      );
}


/// Trigger a software reset, and wait
static inline void software_reset(void) __attribute__ ((noreturn));
void software_reset(void)
{
  CCP = CCP_IOREG_gc;
  RST.CTRL |= RST_SWRST_bm;
  for(;;);
}


#endif
