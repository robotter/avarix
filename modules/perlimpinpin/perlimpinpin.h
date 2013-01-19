/** @defgroup perlimpinpin Perlimpinpin
 * @brief Perlimpinpin module
 *
 * Perlimpinpin (shorted as PPP) is a communication protocol. This module
 * handles PPP communications through UART.
 *
 * There are three things to do to use PPP:
 *  - define an implement \ref ppp_filter_cb_t "frame filter"
 *  - initialize a \ref ppp_intf_t "PPP interface"
 *  - call ppp_intf_update() regularly to process pending data
 *
 * The frame filter is called when a frame header is received. It returns the
 * \ref ppp_payload_handler_t "handler" used to process the payload.
 *
@code
ppp_payload_handler_t *ppp_filter(ppp_intf_t *intf)
{
  if(intf->rstate.header.dst != 0xFF && intf->rstate.header.dst != intf->addr) {
    return NULL; // ignore frames we're not a recipient of
  }
  switch(intf->rstate.header.pltype) {
    case PPP_TYPE_SYSTEM:
      return ppp_payload_handler_system; // use predefined handler
    case PPP_TYPE_DUMMY:
      return my_dummy_handler; // custom handler
    default:
      return NULL; // ignore frames with unhandled type
  }
}

int main(void)
{
  // usual initialization, including UART
  clock_init();
  uart_init();
  PMIC.CTRL |= (PMIC_LOLVLEN_bm|PMIC_MEDLVLEN_bm|PMIC_HILVLEN_bm);

  // initialize the PPP interface
  ppp_intf_t intf;
  intf.filter = ppp_filter; // our frame filter
  intf.uart = uartC0; // UART from the UART module
  intf.addr = 0x10; // interface node address
  ppp_intf_init(&intf);

  // send a system RESET to signal that we have booted
  ppp_send_system_reset(&intf);
  for(;;) {
    // handle input frames
    ppp_intf_update(&intf);
  }
}
@endcode
 */
//@{
/**
 * @file
 * @brief Perlimpinpin definitions
 */
#ifndef PERLIMPINPIN_H__
#define PERLIMPINPIN_H__

#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>
#include "payloads.h"
#include "perlimpinpin_config.h"


#ifndef DOXYGEN

#ifndef PPP_PAYLOAD_BUF_SIZE
# define PPP_PAYLOAD_BUF_SIZE  0
#elif PPP_PAYLOAD_BUF_SIZE > 255
# error PPP_PAYLOAD_BUF_SIZE must not be greater than 255
#endif

#endif


/// Perlimpinpin frame header
typedef struct {
  uint16_t plsize;  ///< length of payload data
  uint8_t src;  ///< source address
  uint8_t dst;  ///< destination address
  uint8_t pltype;  ///< payload type

} __attribute__((__packed__)) ppp_header_t;


#ifdef DOXYGEN
// forward declaration tricks for Doxygen
/** @cond skip */
#define ppp_intf_struct
/** @endcond */
#else

typedef struct ppp_intf_struct ppp_intf_t;

#endif

/** @brief Handler to process the frame payload
 *
 * This handler will be called multiple times.
 * It must receive frame data using ppp_recv_frame_data() then
 * ppp_recv_frame_crc().
 *
 * Data should be received without blocking to allow asynchronous behavior.
 *
 * The handler is allowed to modify the header for its internal needs.
 * For instance, it can decrement the \e plsize and use it as a counter.
 *
 * When 0 is returned, payload data must have been read (including CRC).
 * When 1 is returned, handler is set to ppp_payload_handler_drop() to consume
 * the remaining data.
 *
 * @retval -1  not enough data (must be called again later)
 * @retval  0  payload processing is over
 * @retval  1  payload processing aborted
 *
 * @note When 1 is returned, \e plsize must match the remaining payload size to
 * receive.
 */
typedef int8_t ppp_payload_handler_t(ppp_intf_t *intf);

/** @brief Callback to filter received frames
 *
 * This callback is called when a frame header is received.
 * It returns a \ref ppp_payload_handler_t or \e NULL to drop the frame.
 *
 * The received frame header is stored in \c intf->rstate.header.
 */
typedef ppp_payload_handler_t *ppp_filter_cb_t(ppp_intf_t *intf);



/// State of frame being received on an interface
typedef struct {
  /// Currently received header
#ifdef DOXYGEN
  ppp_header_t header;
#else
  union {
    ppp_header_t header;
    uint8_t header_buf[sizeof(ppp_header_t)];
  };
  uint8_t pos;  ///< number of received header and CRC bytes
  uint16_t crc;  ///< computed CRC (for both header and payload)
#endif

  /** @brief User data for payload processing
   *
   * Custom pointer to track status of currently parsed payload.
   * It is intended to be updated by the payload handler or the frame filter.
   *
   * Automatically reset before processing a new frame.
   */
  void *udata;

#if PPP_PAYLOAD_BUF_SIZE || DOXYGEN
  /** @brief Received payload data
   *
   * This buffer is intended to be used with ppp_recv_frame_payload()
   * but can be used freely. Especially, \ref payload_pos can be updated to
   * write pending payload data back to the beginning of the buffer.
   *
   * @note Not available when \ref PPP_PAYLOAD_BUF_SIZE is unset or null.
   */
  uint8_t payload[PPP_PAYLOAD_BUF_SIZE];
  /** @brief Current index in \ref payload
   *
   * Automatically reset before processing a new frame.
   *
   * @note Not available when \ref PPP_PAYLOAD_BUF_SIZE is unset or null.
   */
  uint8_t payload_pos;
#endif

  /** @brief Current payload handler or \e NULL
   *
   * When \e NULL, indicates that no payload is being received.
   *
   * The current payload handler can be updated while receiving the payload.
   * However, it must not be reset to \e NULL.
   * Also, make sur to properly update or reset other fields before.
   */
  ppp_payload_handler_t *payload_handler;

} ppp_intf_rstate_t;

/// State of frame currenctly sent on an interface
typedef struct {
#ifndef DOXYGEN
  uint16_t crc;  ///< computed CRC
#endif

} ppp_intf_wstate_t;


/** @brief Perlimpinpin node interface
 */
typedef struct ppp_intf_struct {
  uart_t *uart;  ///< UART used by the interface
  uint8_t addr;  ///< interface address
  ppp_filter_cb_t *filter;  ///< callback to filter received frames
  ppp_intf_rstate_t rstate;  ///< state of currently received frame
  ppp_intf_wstate_t wstate;  ///< state of currently sent frame

} ppp_intf_t;



/** @brief Initialize an interface
 *
 * The \ref ppp_intf_t::uart "uart" and \ref ppp_intf_t::addr "addr" fields
 * must be set before using the interface.
 */
void ppp_intf_init(ppp_intf_t *intf);

/** @brief Handle all pending events on an interface
 *
 * If a frame header is received, the \ref ppp_intf_t::filter
 * "interface's filter" is called. Payload data must be received using
 * \ref ppp_recv_frame_data() then CRC checked using ppp_recv_frame_crc().
 *
 * @note This function is not thread-safe. It must not be called when frame
 * data are being received.
 */
void ppp_intf_update(ppp_intf_t *intf);

/** @brief Receive a single byte of frame data, without blocking
 *
 * @return The received value or -1 if it would block.
 */
int ppp_recv_frame_data(ppp_intf_t *intf);

/** @brief Receive and check CRC of the received frame
 *
 * This function must be called after reading all frame data and before
 * returning from \ref ppp_intf_t::filter "interface's filter".
 *
 * Always returns -1 when the two CRC bytes have not been read yet, even if the
 * first byte mismatches.
 *
 * @retval  -1  not enough data (must be called again later)
 * @retval   0  CRC mismatch
 * @retval   1  CRC match
 *
 * @note Read CRC bytes are XOR-ed with the computed CRC which is thus modified
 * as a side effect.
 */
int8_t ppp_recv_frame_crc(ppp_intf_t *intf);

#if PPP_PAYLOAD_BUF_SIZE || DOXYGEN
/** @brief Receive frame payload into \ref ppp_intf_rstate_t::payload "payload" field
 *
 * For short payloads it is usually needed to load all or parts of the payload
 * before processing it. This function loads the payload is loaded into
 * \c intf->rstate.payload and updates \c intf->rstate.payload_pos accordingly.
 *
 * Date is read until \ref ppp_intf_rstate_t::payload "payload_pos" reaches \e n.
 *
 * @note It is the responsibility of the caller to ensure that data to read
 * will fit in the payload buffer.
 *
 * @return \e true if all data have been read, \e false otherwise.
 *
 * @note Not available when \ref PPP_PAYLOAD_BUF_SIZE is unset or null.
 */
bool ppp_recv_frame_payload(ppp_intf_t *intf, uint8_t n);

/** @brief Receive remaining payload data and CRC
 *
 * Receive remaining payload data using ppp_recv_frame_payload() and check the
 * CRC.
 *
 * By decrementing header's \e plsize before calling this method it is possible
 * to read the remaining part of a payload.
 *
 * @note It is the responsibility of the caller to ensure that data to read
 * will fit in the payload buffer.
 *
 * @retval  -1  not enough data (must be called again later)
 * @retval   0  CRC mismatch, frame is invalid
 * @retval   1  payload successfully received
 *
 * @note Not available when \ref PPP_PAYLOAD_BUF_SIZE is unset or null.
 */
int8_t ppp_recv_frame_payload_all(ppp_intf_t *intf);
#endif

/** @brief Payload handler to drop frames
 *
 * This is the default handler if \ref ppp_filter_cb_t "filter" returned
 * \e NULL.
 *
 * It can be used to drop the remaining part of a frame, for instance after
 * reading a byte with an unexpected value.
 *
 * @note This handler never returns 1.
 */
int8_t ppp_payload_handler_drop(ppp_intf_t *intf);


/** @brief Send a complete frame
 *
 * The whole frame is sent: header, header CRC, data and payload CRC.
 * \e data size must match frame's \e plsize.
 */
void ppp_send_frame(ppp_intf_t *intf, const ppp_header_t *header, const void *data);

/** @brief Send a frame header
 *
 * Send frame header and header CRC, reset CRC.
 *
 * This function can be used together with ppp_send_frame_data() and
 * ppp_send_frame_crc() to send frame data by chunks, which is useful for large
 * payloads.
 */
void ppp_send_frame_header(ppp_intf_t *intf, const ppp_header_t *header);

/** @brief Send frame data
 *
 * Send \e size bytes of payload data, update payload CRC.
 *
 * This function is intended to be called after ppp_send_frame_header().
 */
void ppp_send_frame_data(ppp_intf_t *intf, const void *data, uint8_t size);

/** @brief Send frame's payload CRC
 *
 * This function is intended to be called after sending \e plsize bytes using
 * ppp_send_frame_data(). No check is made to enforce this.
 */
void ppp_send_frame_crc(ppp_intf_t *intf);


#endif
//@}
