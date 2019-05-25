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


inline uint16_t rome_frame_get_crc(const rome_frame_t *frame)
{
  const uint8_t *const p = &frame->_data[frame->plsize];
  return *(uint16_t const*)p;
}

static uint16_t rome_compute_crc(const rome_frame_t *frame)
{
  uint16_t crc = 0xffff;
  for(uint8_t i = 0; i < 2+frame->plsize; ++i) {
    crc = _crc_ccitt_update(crc, ((const uint8_t*)&frame->plsize)[i]);
  }
  return crc;
}


void rome_reader_init(rome_reader_t *reader, uart_t *uart)
{
  reader->uart = uart;
  reader->pos = 0;
  // set the first frame's byte to the start byte once and for all
  reader->buf[0] = ROME_START_BYTE;
}


rome_frame_t *rome_reader_read(rome_reader_t *reader)
{
  uint8_t *const pos = &reader->pos;

  for(;;) {
    // wait for a start byte
    while(*pos < 1) {
      switch(uart_recv_nowait(reader->uart)) {
        case -1:
          return NULL;
        case ROME_START_BYTE:
          (*pos)++;
          break;
        default:
          *pos = 0;
      }
    }

    // read header, payload and CRC
    const uint8_t pos_end = 3 + reader->frame.plsize + 2;
    while(*pos < pos_end) {
      int ret = uart_recv_nowait(reader->uart);
      if(ret == -1) {
        return NULL;
      }
      reader->buf[*pos] = ret;
      (*pos)++;
    }

    // reset state for the next frame
    *pos = 0;

    // if CRC matches, return the frame
    if(rome_compute_crc(&reader->frame) == rome_frame_get_crc(&reader->frame)) {
      return &reader->frame;
    }
  }

  return NULL;
}


const rome_frame_t *rome_parse_frame(const uint8_t data[], uint8_t len)
{
  if(len < 3+0+2) {
    return NULL;  // not enough bytes for a valid frame
  }
  if(data[0] != ROME_START_BYTE) {
    return NULL;  // wrong start byte
  }
  if(len != 3+data[1]+2) {
    return NULL;  // wrong plsize
  }

  const rome_frame_t *const frame = (const rome_frame_t*)data;
  if(rome_compute_crc(frame) != rome_frame_get_crc(frame)) {
    return NULL;  // CRC mismatch
  }

  return frame;
}

void rome_finalize_frame(rome_frame_t *frame)
{
  frame->start = ROME_START_BYTE;
  const uint8_t *const p = &frame->_data[frame->plsize];
  *(uint16_t*)p = rome_compute_crc(frame);
}


void rome_send_uart(uart_t *uart, const rome_frame_t *frame)
{
  if(frame->mid == 0) {
    return;
  }

  ROME_SEND_INTLVL_DISABLE() {
    uart_send_buf(uart, (const uint8_t*)frame, 3 + frame->plsize + 2);
  }
}

#ifdef ROME_ENABLE_XBEE_API

void rome_send_xbee(xbee_intf_t *xbee, uint16_t addr, const rome_frame_t *frame)
{
  if(frame->mid == 0) {
    return;
  }

  xbee_send(xbee, addr, (const uint8_t*)frame, 3 + frame->plsize + 2);
}

#endif


#ifdef ROME_ACK_MIN

#define ROME_ACK_COUNT  ((ROME_ACK_MAX)-(ROME_ACK_MIN)+1)

/// Array of ACK values, value is true when an ACK is expected
static uint8_t rome_active_acks[ROME_ACK_COUNT];

uint8_t rome_next_ack(void)
{
  static uint8_t ack = (ROME_ACK_MAX); // MAX so that MIN is the first value to be used
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
      ack = ack == (ROME_ACK_MAX) ? (ROME_ACK_MIN) : ack+1;
      if(!rome_active_acks[ack-(ROME_ACK_MIN)]) {
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
  return rome_active_acks[ack-(ROME_ACK_MIN)];
}

void rome_free_ack(uint8_t ack)
{
  rome_active_acks[ack-(ROME_ACK_MIN)] = false;
}


// templatize
#define ROME_SENDWAIT_FUNCTION { \
  for(;;) { \
    /* send the frame with a new ACK value */\
    uint8_t ack = rome_next_ack(); \
    frame->_data[0] = ack; \
    rome_finalize_frame(frame); \
    rome_send(dst, frame); \
    /* wait ack */\
    uint32_t tend = uptime_us() + ROME_ACK_TIMEOUT_US; \
    do { \
      ROME_SEND_INTLVL_DISABLE() { \
        if(!rome_active_acks[ack-(ROME_ACK_MIN)]) { \
          return; \
        } \
        idle(); \
      } \
    } while(uptime_us() < tend); \
  } \
}

void rome_sendwait_uart(uart_t *dst, rome_frame_t *frame)  ROME_SENDWAIT_FUNCTION
#ifdef ROME_ENABLE_XBEE_API
void rome_sendwait_xbee_dst(rome_xbee_dst_t dst, rome_frame_t *frame)  ROME_SENDWAIT_FUNCTION
#endif

#endif

///@endcond
