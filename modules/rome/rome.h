/** @defgroup rome ROME
 * @brief ROME module
 *
 * ROME is a communication protocol. This module handles ROME communications
 * through UART or XBee API.
 *
 *  - When using UART, frames are read using a \ref rome_reader_t and \ref
 *    rome_reader_read().
 *  - When using XBee API, frames can be parsed using \ref rome_parse_frame().
 *
 * In both cases, frames can be sent using the appropriated `rome_send_*()`
 * method or one of the `ROME_SEND_*()` helpers.
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
#include <avarix/internal.h>
#include <uart/uart.h>
#include "rome_config.h"
#include "rome/rome_msg.h"
#ifdef ROME_ENABLE_XBEE_API
#include <xbee/xbee.h>
#endif


/// Read a frame from an UART, keep current reading state
typedef struct {
  uart_t *uart;  ///< UART interface used to read the frame
  uint8_t pos;  ///< number of received bytes for the current frame
  /// Frame being read
  union {
    rome_frame_t frame;
    uint8_t buf[sizeof(rome_frame_t)];
  };

} rome_reader_t;

/// Initialize a frame reader
void rome_reader_init(rome_reader_t *reader, uart_t *uart);

/** @brief Process input data for a frame reader
 *
 * Return a pointer to the read frame or NULL if no frame has been received.
 */
rome_frame_t *rome_reader_read(rome_reader_t *reader);


/** @brief Parse a single frame from a buffer
 *
 * If the buffer contains exactly a valid frame, return a pointer to this
 * frame. Otherwise, return NULL.
 *
 * @note The returned pointer is bounded to \a data.
 */
const rome_frame_t *rome_parse_frame(const uint8_t data[], uint8_t len);

/// Set frame's start byte and CRC from its payload
void rome_finalize_frame(rome_frame_t *frame);

/** Send a frame to an UART
 *
 * The frame must have been finalized (see rome_finalize_frame()).
 */
void rome_send_uart(uart_t *uart, const rome_frame_t *frame);

# ifdef ROME_ENABLE_XBEE_API

/** Send a frame to an XBee address
 *
 * The frame must have been finalized (see rome_finalize_frame()).
 */
void rome_send_xbee(xbee_intf_t *xbee, uint16_t addr, const rome_frame_t *frame);

/** Broadcast a frame to an XBee interface
 *
 * The frame must have been finalized (see rome_finalize_frame()).
 */
inline void rome_send_xbee_broadcast(xbee_intf_t *xbee, const rome_frame_t *frame)
{
  rome_send_xbee(xbee, XBEE_BROADCAST, frame);
}

/// ROME destination for a specific XBee address
typedef struct {
  xbee_intf_t *xbee;
  uint16_t addr;
} rome_xbee_dst_t;

/// Return a rome_xbee_dst_t for an XBee and address
#define ROME_XBEE_DST(xbee,addr)  ((rome_xbee_dst_t){ (xbee), (addr) })

/// Send a frame to an XBee destination
inline void rome_send_xbee_dst(rome_xbee_dst_t dst, const rome_frame_t *frame)
{
  rome_send_xbee(dst.xbee, dst.addr, frame);
}

#endif

#ifdef DOXYGEN

/// Generic macro to send a frame
# define rome_send(dst, frame)

#else

# ifdef ROME_ENABLE_XBEE_API
#  define rome_send(dst, frame) \
    _Generic((dst) \
             , uart_t*: rome_send_uart \
             , xbee_intf_t*: rome_send_xbee_broadcast \
             , rome_xbee_dst_t: rome_send_xbee_dst \
             )(dst, frame)
# else
#  define rome_send  rome_send_uart
# endif

#endif


/// Reply to a frame with a ACK message
#define rome_reply_ack(intf, frame)  ROME_SEND_ACK((intf), (frame)->_data[0])

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

/** @brief Send a frame over UART until an ACK is received
 *
 * The frame must be an order frame.
 * Frame's ACK value is updated before sending.
 */
void rome_sendwait_uart(uart_t *uart, rome_frame_t *frame);

#ifdef ROME_ENABLE_XBEE_API

/** @brief Send a frame to an XBee address until an ACK is received
 *
 * The frame must be an order frame.
 * Frame's ACK value is updated before sending.
 */
void rome_sendwait_xbee_dst(rome_xbee_dst_t dst, rome_frame_t *frame);

#endif

#ifdef DOXYGEN

/// Generic macro to send a frame until an ACK is received
#define rome_sendwait(dst, frame)

#else

# ifdef ROME_ENABLE_XBEE_API
#  define rome_sendwait(dst, frame) \
    _Generic((dst) \
             , uart_t*: rome_sendwait_uart \
             , rome_xbee_dst_t: rome_sendwait_xbee_dst \
             )(dst, frame)
# else
#  define rome_sendwait  rome_sendwait_uart
# endif

#endif

#endif

#endif
//@}
