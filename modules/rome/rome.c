/**
 * @cond internal
 * @file
 */
#include <util/crc16.h>
#include "rome.h"
#ifdef ROME_ACK_MIN
#include <timer/uptime.h>
#include <idle/idle.h>
#endif


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
  if(frame->mid == 0) {
    return;
  }
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


void rome_reply_ack(rome_intf_t *intf, const rome_frame_t *frame)
{
  ROME_SEND_ACK(intf, frame->_data[0]);
}

#ifdef ROME_ACK_MIN

#define ROME_ACK_COUNT  (ROME_ACK_MAX-ROME_ACK_MIN+1)

/// Array of ACK values, value is true when an ACK is expected
static uint8_t rome_active_acks[ROME_ACK_COUNT];

uint8_t rome_next_ack(void)
{
  static uint8_t ack = ROME_ACK_MAX; // MAX so that MIN is the first value to be used
  uint8_t ret;
  // an available value should be found quickly
  // it's not risky to lock the loop
  ROME_SEND_INTLVL_DISABLE() {
    // if the whole ACK range is used, an uint8_t is not enough
#if ROME_ACK_COUNT > 255
    uint16_t i;
#else
    uint8_t i;
#endif
    for(i=0; i<ROME_ACK_COUNT; i++) {
      ack = ack == ROME_ACK_MAX ? ROME_ACK_MIN : ack+1;
      if(!rome_active_acks[ack-ROME_ACK_MIN]) {
        break;
      }
    }
    // also reached if all values are already in use
    ret = ack;
    rome_active_acks[ret] = true;
  }
  return ret;
}

bool rome_ack_expected(uint8_t ack)
{
  return rome_active_acks[ack-ROME_ACK_MIN];
}

void rome_free_ack(uint8_t ack)
{
  rome_active_acks[ack-ROME_ACK_MIN] = false;
}


void rome_sendwait(rome_intf_t *intf, rome_frame_t *frame)
{
  for(;;) {
    // send the frame with a new ACK value
    uint8_t ack = rome_next_ack();
    frame->_data[0] = ack;
    rome_send(intf, frame);
    // wait for the ACK
    uint32_t tend = uptime_us() + ROME_ACK_TIMEOUT_US;
    do {
      ROME_SEND_INTLVL_DISABLE() {
        if(!rome_active_acks[ack-ROME_ACK_MIN]) {
          return;
        }
        idle();
      }
    } while(uptime_us() < tend);
  }
}

#endif

///@endcond
