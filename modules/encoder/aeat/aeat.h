/** @defgroup aeat AEAT encoder
 * @brief Avago's AEAT-6010 and AEAT-6012 encoders
 */
//@{
/**
 * @file
 * @brief Definitions for AEAT encoders
 */
#ifndef ENCODER_AEAT_H__
#define ENCODER_AEAT_H__

#include <avr/io.h>
#include <stdint.h>
#include <avarix/portpin.h>


/** @brief AEAT encoder data
 * @note Fields are private and should not be accessed directly.
 */
typedef struct {
  portpin_t cspp;  ///< port pin of the inversed CS pin
  uint16_t capture;  ///< last captured value
  int32_t value;  ///< current encoder value
} aeat_t;



/** @brief Initialize enc SPI
 *
 * This function must be called once before using encoders.
 */
void aeat_spi_init(void);

/** @brief Initialize an encoder
 *
 * @param enc  encoder to initialize
 * @param cspp  port pin of the CS pin
 */
void aeat_init(aeat_t *enc, portpin_t cspp);

/// Update encoder value, should be called often
void aeat_update(aeat_t *enc);

/// Get current encoder value
int32_t aeat_get_value(aeat_t *enc);

/// Set encoder value
void aeat_set_value(aeat_t *enc, int32_t v);


#endif
//@}
