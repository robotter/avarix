/** @defgroup quadra Quadra
 * @brief Encoder base on quadrature decoder
 */
//@{
/**
 * @file
 * @brief Quadrature module definitions
 */
#ifndef ENCODER_QUADRA_H__
#define ENCODER_QUADRA_H__

#include <avr/io.h>
#include <avarix/portpin.h>


/** Quadrature encoder data
 * @note Fields are private and should not be accessed directly.
 */
typedef struct {
  TC1_t *tc;  ///< timer used to decode the quadrature signal
  uint16_t capture;  ///< last captured value
  int32_t value;  ///< current encoder value
} quadra_t;


/** @brief Initialize quadrature encoder
 *
 * @param enc  quadrature encoder to initialize
 * @param tc  timer to use (can also be a pointer to a \e TC0_t)
 * @param evch  event channel to use (0, 2 or 4)
 * @param pp0  port pin of the reference encoder signal
 * @param pp90  port pin of the phase shifted encoder signal
 * @param samples  length of digital filtering (1 to 8)
 *
 * A quadrature decoder uses two timer channels and one event channel. Since it
 * only requires two channels it is better to use a timer of type 1. However,
 * timers of type 0 can be used too.
 *
 * Input signal must use pins valid as event source (port A to F).
 */
void quadra_init(quadra_t *enc, TC1_t *tc, uint8_t evch, portpin_t pp0, portpin_t pp90, uint8_t samples);

/// Update encoder value, should be called often
void quadra_update(quadra_t *enc);

/// Get the current decoder value
int32_t quadra_get_value(quadra_t *enc);

/// Reset the decoder value
void quadra_set_value(quadra_t *enc, int32_t v);


#endif
//@}
