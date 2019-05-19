/** @defgroup xbee Xbee
 * @brief XBee API module
 *
 * Receive and send XBee API frames through an UART.
 */
//@{
/**
 * @file
 * @brief XBee module definitions
 */
#ifndef AVARIX_XBEE_H__
#define AVARIX_XBEE_H__

#include <stdint.h>
#include <avarix/internal.h>
#include <uart/uart.h>
#include "xbee_config.h"


#ifdef DOXYGEN
// Doxygen trick to have the typedef name for struct documentation
/** @cond skip */
#define xbee_intf_struct xbee_intf_t
/** @endcond */
#endif

typedef struct xbee_intf_struct xbee_intf_t;

/// XBee API broadcast address
#define XBEE_BROADCAST  0xffff

typedef enum {
  XBEE_ID_TX16 = 0x01,
  XBEE_ID_RX16 = 0x81,

} xbee_api_id_t;

/// XBee API frame (suitable for RX16 only)
typedef struct {
  uint16_t length;  ///< length for API frame
  uint8_t api_id;  ///< API identifier
  union {
    struct {
      uint16_t addr_be; // big endian
      uint8_t rssi;
      uint8_t options;
      uint8_t data[100];
    } rx16;
  };
} __attribute__((__packed__)) xbee_frame_t;

/// Frame handler, process received RX packets
typedef void xbee_handler_t(xbee_intf_t *intf, const xbee_frame_t *frame);

/// State of frame being received on an interface
typedef struct {
  /// frame data
  union {
    xbee_frame_t frame;
    uint8_t buf[sizeof(xbee_frame_t)];
  };
  uint8_t pos;  ///< number of received bytes for the current frame
  uint8_t checksum;  ///< computed checksum

} xbee_rstate_t;

/// XBee API interface
struct xbee_intf_struct {
  uart_t *uart;  ///< UART used by the interface
  xbee_handler_t *handler;  ///< frame handler
  xbee_rstate_t rstate;  ///< state of frame being received (internal)
};


/** @brief Initialize an interface for a given UART interface
 *
 * The \ref xbee_intf_t::handler "handler" field should be set before using the
 * interface.
 */
void xbee_intf_init(xbee_intf_t *intf, uart_t *uart);

/** @brief Process input data on an interface
 *
 * The frame handler is called for each received frame.
 */
void xbee_handle_input(xbee_intf_t *intf);

/** Send a data an interface
 *
 * Data is split into multiple API frames if needed. 
 */
void xbee_send(xbee_intf_t *intf, uint16_t addr, const uint8_t data[], uint8_t len);

#endif
//@}

