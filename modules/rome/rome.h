/** @defgroup rome ROME
 * @brief ROME module
 *
 * ROME is a communication protocol. This module handles ROME communications
 * through UART.
 *
 * The following things are needed to use ROME:
 *  - define and implement a \ref rome_handler_t "frame handler"
 *  - initialize a \ref rome_intf_t "ROME interface"
 *  - call rome_intf_update() regularly to process input data
 *
 * The frame handler is set on the interface and called when a valid frame
 * has been received.
 *
 * @par Orders and ACKs
 *
 * When acknowledgement is needed, an ACK value is to the frame. This value is
 * incremented for each sent ACK-able order to be unique. The recipient sent
 * back the ACK value received with the order to acknowledge, using
 * \ref rome_reply_ack(). This system only works if the caller does not send
 * another order before the first one has been acknowledged. For this reason,
 * ACK values are not accessed atomically, since it would not work anyway.
 *
 * Moreover, if ACKs need to be forwarded from one interface to another, the
 * range of ACK values must split between all order senders to avoid
 * overlapping ACK values. \ref ROME_ACK_MIN and \ref ROME_ACK_MAX define the
 * range of ACK values to use.
 *
 * Currently used ACK values are stored in an array. This allows to keep track
 * of which orders have been acknowledged.
 */
//@{
/**
 * @file
 * @brief ROME module definitions
 */
#ifndef ROME_H__
#define ROME_H__

#include <stdint.h>
#include <stdbool.h>
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

/// Reply to a frame with a ACK message
void rome_reply_ack(rome_intf_t *intf, const rome_frame_t *frame);

#if (defined DOXYGEN) || (defined ROME_ACK_MIN)

/// Get the next ACK value to be used for a sent message
uint8_t rome_next_ack(void);

#ifdef DOXYGEN
/// Return wheter a ACK value is in our range
bool rome_ack_in_range(ack);
#else
// several cases, to avoid "comparison is always true" warnings
#ifndef ROME_ACK_MIN
# define rome_ack_in_range(ack)  false
#elif ROME_ACK_MIN > 0 && ROME_ACK_MAX < 255
# define rome_ack_in_range(ack)  ((ack) >= ROME_ACK_MIN && (ack) <= ROME_ACK_MAX)
#elif ROME_ACK_MIN > 0
# define rome_ack_in_range(ack)  ((ack) >= ROME_ACK_MIN)
#else // ROME_ACK_MAX < 255
# define rome_ack_in_range(ack)  ((ack) <= ROME_ACK_MAX)
#endif
#endif

/// Return whether a given ACK is expected
bool rome_ack_expected(uint8_t ack);

/// Free an ACK value
void rome_free_ack(uint8_t ack);

/** @brief Send a frame until an ACK is received
 *
 * The frame must be an order frame.
 * Frame's ACK value is updated before sending.
 */
void rome_sendwait(rome_intf_t *intf, rome_frame_t *frame);

#endif

#endif
//@}
