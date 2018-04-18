/** @defgroup ws2812b WS2812B intelligent control LED
 * @brief WorldSemi WS2812B intelligent led
 *
 * This module handle a daisy-chain of WS2812B intelligent LEDs.
 *
 */
//@{
/**
 * @file
 * @brief Definitions for WS2812B led
 */

#ifndef WS2812B_H
#define WS2812B_H

#include <avr/io.h>

typedef struct {


} ws2812b_t;

/** @brief Initialize module
 * @param dout pin where the ws2812b daisy-chain is plugged
 */
void ws2812b_init(ws2812b_t *ws, TC0_t *tc, char channel);

#endif/*WS2812B_H*/
