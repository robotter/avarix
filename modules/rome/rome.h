/** @defgroup rome ROME
 * @brief ROME module
 *
 * ROME is a communication protocole. This module handles Rome communications
 * through UART.
 *
 * The following things are needed to use ROME:
 *  - define and implement a \ref rome_handler_t "frame handler"
 *  - initialize a \ref rome_intf_t "ROME interface"
 *  - call rome_intf_update() regularly to process input data
 *
 * The frame handler is set on the interface and called when a valid frame
 * has been received.
 */
//@{
/**
 * @file
 * @brief ROME module definitions
 */
#ifndef ROME_H__
#define ROME_H__

#include <stdint.h>
#include <uart/uart.h>
#include "rome_config.h"
#include "rome/rome_msg.h"


#ifdef DOXYGEN
// Doxygen trick to have the typedef name for struct documentation
/** @cond skip */
#define rome_intf_struct rome_intf_t
/** @endcond */
#endif

typedef struct rome_intf_struct rome_intf_t;


/// Frame handler, process received ROME messages
typedef void rome_handler_t(rome_intf_t *intf, const rome_frame_t *frame);


/// State of frame being received on an interface
typedef struct {
  /// frame data
  union {
    rome_frame_t frame;
    uint8_t buf[sizeof(rome_frame_t)];
  };
  uint8_t pos;  ///< number of received bytes for the current frame
  uint16_t crc;  ///< computed CRC

} rome_rstate_t;

/// ROME interface
struct rome_intf_struct {
  uart_t *uart;  ///< UART used by the interface
  rome_handler_t *handler;  ///< frame handler
  rome_rstate_t rstate;  ///< state of frame being received (internal)
};


/** @brief Initialize an interface
 *
 * The \ref rome_intf_t::uart "uart" field must be set before using the
 * interface. The \ref rome_intf_t::handler "handler" field should be set too.
 */
void rome_intf_init(rome_intf_t *intf);

/** @brief Process input data on an interface
 *
 * The frame handler is called for each received frame.
 */
void rome_handle_input(rome_intf_t *intf);

/// Send a frame on an interface
void rome_send(rome_intf_t *intf, const rome_frame_t *frame);


#endif
//@}
