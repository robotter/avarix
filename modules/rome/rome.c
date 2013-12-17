/**
 * @cond internal
 * @file
 */
#include <util/crc16.h>
#include "rome.h"


/// Frame start byte
#define ROME_START_BYTE  0x52 // 'R'


#ifdef ROME_SEND_INTLVL
# define ROME_SEND_INTLVL_DISABLE()  INTLVL_DISABLE_BLOCK(ROME_SEND_INTLVL)
#else
# define ROME_SEND_INTLVL_DISABLE()
#endif


void rome_intf_init(rome_intf_t *intf)
{
  intf->rstate.pos = 0;
  intf->rstate.crc = 0xffff;
}


void rome_handle_input(rome_intf_t *intf)
{
  uint8_t *const pos = &intf->rstate.pos;
  uint16_t *const crc = &intf->rstate.crc;

  for(;;) {
    // start byte
    while(*pos < 1) {
      switch(uart_recv_nowait(intf->uart)) {
        case -1:
          return;
        case ROME_START_BYTE:
          (*pos)++;
          break;
        default:
          *pos = 0;
      }
    }

    // payload size and message ID
    while(*pos < 3) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      intf->rstate.buf[*pos-1] = ret;
      *crc = _crc_ccitt_update(*crc, ret);
      (*pos)++;
    }

    // payload data
    uint8_t plsize = intf->rstate.frame.plsize;
    while(*pos < 3+plsize) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      intf->rstate.buf[*pos-1] = ret;
      *crc = _crc_ccitt_update(*crc, ret);
      (*pos)++;
    }

    // CRC
    if(*pos != 3+plsize+1) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      *crc ^= (ret & 0xff);
      (*pos)++;
    }
    if(*pos != 3+plsize+2) {
      int ret = uart_recv_nowait(intf->uart);
      if(ret == -1) {
        return;
      }
      *crc ^= (ret & 0xff) << 8;
      (*pos)++;
    }

    // if CRC matches, handle the frame
    if(*crc == 0) {
      intf->handler(intf, &intf->rstate.frame);
    }

    // reset state for the next frame
    *pos = 0;
    *crc = 0xffff;
  }
}


void rome_send(rome_intf_t *intf, const rome_frame_t *frame)
{
  ROME_SEND_INTLVL_DISABLE() {
    uint16_t crc = 0xffff;
    uart_send(intf->uart, ROME_START_BYTE);
    uart_send(intf->uart, frame->plsize);
    crc = _crc_ccitt_update(crc, frame->plsize);
    uart_send(intf->uart, frame->mid);
    crc = _crc_ccitt_update(crc, frame->mid);
    uint8_t i;
    for(i=0; i<frame->plsize; i++) {
      uart_send(intf->uart, frame->_data[i]);
      crc = _crc_ccitt_update(crc, frame->_data[i]);
    }
    uart_send(intf->uart, crc & 0xff);
    uart_send(intf->uart, (crc >> 8) & 0xff);
  }
}


