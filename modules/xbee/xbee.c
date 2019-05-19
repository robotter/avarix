/**
 * @cond internal
 * @file
 */
#include "xbee.h"


/// Frame start byte
#define XBEE_START_BYTE  0x7E


#ifdef XBEE_SEND_INTLVL
# define XBEE_SEND_INTLVL_DISABLE()  INTLVL_DISABLE_BLOCK(XBEE_SEND_INTLVL)
#else
# define XBEE_SEND_INTLVL_DISABLE()
#endif


void xbee_intf_init(xbee_intf_t *intf, uart_t *uart)
{
  intf->rstate.pos = 0;
  intf->rstate.checksum = 0xff;
  intf->uart = uart;
}


void xbee_handle_input(xbee_intf_t *intf)
{
  uint8_t *const pos = &intf->rstate.pos;
  uint8_t *const checksum = &intf->rstate.checksum;

  for(;;) {
    // start byte
    while(*pos < 1) {
      switch(uart_recv_nowait(intf->uart)) {
        case -1:
          return;
        case XBEE_START_BYTE:
          (*pos)++;
          break;
        default:
          *pos = 0;
      }
    }

    // length (big endian)
    if(*pos < 2) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      intf->rstate.buf[*pos-1+1] = ret; // MSB
      (*pos)++;
    }
    if(*pos < 3) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      intf->rstate.buf[*pos-1-1] = ret; // LSB
      (*pos)++;
    }

    // skip unhandled frames right now (too long)
    if(intf->rstate.frame.length > sizeof(xbee_frame_t)-2) {
      while(*pos < 3+intf->rstate.frame.length+1) {
        int ret = uart_recv_nowait(intf->uart);
        if(ret == -1) {
          return;
        }
        (*pos)++;
      }
    } else {
      // frame data
      const uint8_t length = intf->rstate.frame.length;  // truncate to 8-bit
      while(*pos < 3+length) {
        int ret = uart_recv_nowait(intf->uart);
        if(ret == -1) {
          return;
        }
        intf->rstate.buf[*pos-1] = ret;
        *checksum -= ret;
        (*pos)++;
      }

      // checksum
      if(*pos != 3+length+1) {
        int ret = uart_recv_nowait(intf->uart);
        if(ret == -1) {
          return;
        }
        *checksum -= ret;  // compare checksum
        (*pos)++;
      }

      // if CRC matches, handle the frame
      if(*checksum == 0) {
        intf->handler(intf, &intf->rstate.frame);
      }
    }

    // reset state for the next frame
    *pos = 0;
    *checksum = 0xff;
  }
}


void xbee_send(xbee_intf_t *intf, uint16_t addr, const uint8_t data[], uint8_t len)
{
  while(len) {
    uint8_t data_len = len > 100 ? 100 : len;
    uint8_t checksum = 0xff;
    XBEE_SEND_INTLVL_DISABLE() {
      uart_send(intf->uart, XBEE_START_BYTE);
      uart_send(intf->uart, 0);
      uart_send(intf->uart, data_len + 5);  // date_len + 5 <= 105
#define SEND_BYTE_FOR_CHECKSUM(v) do { \
        const uint8_t v_ = (v); \
        uart_send(intf->uart, v); checksum -= v_; \
      } while(0)
      SEND_BYTE_FOR_CHECKSUM(0x01);  // API identifier
      SEND_BYTE_FOR_CHECKSUM(0);  // no response frame
      SEND_BYTE_FOR_CHECKSUM(addr >> 8);
      SEND_BYTE_FOR_CHECKSUM(addr);
      SEND_BYTE_FOR_CHECKSUM(addr == XBEE_BROADCAST ? 0x04 : 0);
      for(uint8_t i=0; i<data_len; ++i) {
        SEND_BYTE_FOR_CHECKSUM(data[i]);
      }
#undef SEND_BYTE
      uart_send(intf->uart, checksum);
    }
    len -= data_len;
    data += data_len;
  }
}

///@endcond
