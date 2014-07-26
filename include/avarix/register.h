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
inline void ccp_io_write(volatile uint8_t* addr, uint8_t value)
{
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


#endif
