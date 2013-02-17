/** @defgroup quadra Quadra
 * @brief Quadrature decoder
 *
 * This module is intended to be used for motor coders.
 *
 * Decoded value is a 16-bit value which can overflow or underflow.
 */
//@{
/**
 * @file
 * @brief Quadrature module definitions
 */
#ifndef QUADRA_H__
#define QUADRA_H__

#include <avr/io.h>


/// Quadrature decoder data
typedef struct quadra_struct quadra_t;


/** @brief Initialize quadrature decoder
 *
 * @param q  quadrature decoder to initialize
 * @param tc  timer to use (can also be a pointer to a \e TC0_t)
 * @param evch  event channel to use (0, 2 or 4)
 * @param port0  port of the reference encoder signal
 * @param pin0  pin of the reference encoder signal
 * @param port90  port of the phase shifted encoder signal
 * @param pin90  pin of the phase shifted encoder signal
 * @param samples  length of digital filtering (1 to 8)
 *
 * A quadrature decoder uses two timer and one event channel. Since it only
 * requires two channels it is better to use a timer of type 1. However, timers
 * of type 0 can be used too.
 *
 * Input signal must use pins valid as event source (port A to F).
 */
void quadra_init(quadra_t *q, TC1_t *tc, uint8_t evch, PORT_t *port0, uint8_t pin0, PORT_t *port90, uint8_t pin90, uint8_t samples);

/// Get the current decoder value
uint16_t quad_get(quadra_t *q);

/// Reset the decoder value
void quad_reset(quadra_t *q, uint16_t value);


#endif
//@}
