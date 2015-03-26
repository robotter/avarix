/**
 * @file
 * @brief Tools to work with interrupt levels
 *
 * Interrupt levels are 2-bit values used by many modules, especially to
 * configure level of their interrupts.
 *
 * Interrupt level bitmasks are 3-bit values (1 bit per level), used for
 * instance in the PMIC.CTRL register. They are used by macros that enable or
 * disable interrupts (\e lvlbm parameter).
 */
//@{
#ifndef AVARIX_INTLVL_H__
#define AVARIX_INTLVL_H__

#include <avr/io.h>
#include <stdint.h>


/// Interrupt level
typedef enum {
  INTLVL_NONE = 0,
  INTLVL_LO = 1,
  INTLVL_MED = 2,
  INTLVL_HI = 3,

} intlvl_t;


/// Get bitmask for given interrupt level
#define INTLVL_BM(lvl)  (1 << ((lvl)-1))
/// Get bitmask for levels lower than or equal to given interrupt level
#define INTLVL_BM_LO(lvl)  ((1 << (lvl)) - 1)
/// Get bitmask for levels higher than or equal to given interrupt level
#define INTLVL_BM_HI(lvl)  (8 - (1 << ((lvl)-1)))


/// Enable interrupt levels by bitmask
#define INTLVL_ENABLE(lvlbm)  (PMIC.CTRL |= (lvlbm))
/// Disable interrupt levels by bitmask
#define INTLVL_DISABLE(lvlbm)  (PMIC.CTRL &= ~(lvlbm))

/// Enable all interrupt levels
#define INTLVL_ENABLE_ALL()  INTLVL_ENABLE(7)
/// Disable all interrupt levels
#define INTLVL_DISABLE_ALL()  INTLVL_DISABLE(7)

/** @brief Disable interrupts by level in a block of code
 *
 * Create a block of code executed with interrupt levels lower than or equal
 * to \e lvl disabled. Original configuration is restored when exiting the
 * block from any exit path.
 */
#define INTLVL_DISABLE_BLOCK(lvl)  INTLVL_DISABLE_BLOCK_BM(INTLVL_BM_LO(lvl))

/** @brief Enable interrupts by level in a block of code
 *
 * Create a block of code executed with interrupt levels higher than or equal
 * to \e lvl enabled. Original configuration is restored when exiting the
 * block from any exit path.
 */
#define INTLVL_ENABLE_BLOCK(lvl)  INTLVL_ENABLE_BLOCK_BM(INTLVL_BM_HI(lvl))

/// Disable all interrupts in a block of code
#define INTLVL_DISABLE_ALL_BLOCK()  INTLVL_DISABLE_BLOCK(INTLVL_HI)

/// Enable all interrupts in a block of code
#define INTLVL_ENABLE_ALL_BLOCK()  INTLVL_ENABLE_BLOCK(INTLVL_LO)


#ifdef DOXYGEN

/** @brief Disable interrupts by level bitmask in a block of code
 *
 * Equivalent to \ref INTLVL_DISABLE_BLOCK but using a given bitmask.
 */
#define INTLVL_DISABLE_BLOCK_BM(lvlbm)

/** @brief Enable interrupts by level bitmask in a block of code
 *
 * Equivalent to \ref INTLVL_ENABLE_BLOCK but using a given bitmask.
 */
#define INTLVL_ENABLE_BLOCK_BM(lvlbm)

#else

static inline void avarix__restore_lvlen(const uint8_t *lvlbm)
{
  PMIC.CTRL = (PMIC.CTRL & ~7) | *lvlbm;
  asm volatile ("" ::: "memory");
}

#define INTLVL_DISABLE_BLOCK_BM(lvlbm) \
    for(uint8_t avarix__intlvl__ __attribute__((__cleanup__(avarix__restore_lvlen))) = PMIC.CTRL, \
        avarix__tmp__ = (INTLVL_DISABLE(lvlbm), 1); \
        avarix__tmp__; avarix__tmp__=0)

#define INTLVL_ENABLE_BLOCK_BM(lvlbm) \
    for(uint8_t avarix__intlvl__ __attribute__((__cleanup__(avarix__restore_lvlen))) = PMIC.CTRL, \
        avarix__tmp__ = (INTLVL_ENABLE(lvlbm), 1); \
        avarix__tmp__; avarix__tmp__=0)

#endif


#endif
//@}
