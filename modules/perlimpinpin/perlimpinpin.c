/**
 * @cond internal
 * @file
 */
#include <util/crc16.h>
#include "perlimpinpin.h"


/// Frame start byte
#define PPP_START_BYTE  'P'

/// Size of frame header, including the start byte
#define PPP_HEADER_SIZE  (sizeof(ppp_header_t)+1)


void ppp_intf_init(ppp_intf_t *intf)
{
  intf->rstate.pos = 0;
  intf->rstate.crc = 0xffff;
  intf->rstate.udata = NULL;
  intf->rstate.payload_handler = NULL;
#if PPP_PAYLOAD_BUF_SIZE
  intf->rstate.payload_pos = 0;
#endif
}


void ppp_intf_update(ppp_intf_t *intf)
{
  for(;;) {
    uint8_t *pos = &intf->rstate.pos;
    uint16_t *crc = &intf->rstate.crc;

    // start byte
    while( *pos < 1 ) {
      switch( uart_recv_nowait(intf->uart) ) {
        case -1:
          return;
        case PPP_START_BYTE:
          (*pos)++;
          break;
        default:
          *pos = 0;
      }
    }

    // header
    while( *pos < PPP_HEADER_SIZE ) {
      int ret = uart_recv_nowait(intf->uart);
      if( ret == -1 ) {
        return;
      }
      intf->rstate.header_buf[*pos-1] = ret;
      *crc = _crc_ccitt_update(*crc, ret);
      (*pos)++;
    }

    // header CRC
    if( *pos == PPP_HEADER_SIZE ) {
      int ret = uart_recv_nowait(intf->uart);
      if( ret == -1 ) {
        return;
      }
      *crc ^= (ret & 0xff);
      (*pos)++;
    }
    if( *pos == PPP_HEADER_SIZE+1 ) {
      int ret = uart_recv_nowait(intf->uart);
      if( ret == -1 ) {
        return;
      }
      *crc ^= (ret & 0xff) << 8;
      (*pos)++;

      if( *crc != 0 ) {
        // mismatch
        *pos = 0;
        *crc = 0xffff;
        continue;
      }
      // reset CRC for payload
      *crc = 0xffff;
    }

    // filter the frame
    if( intf->rstate.payload_handler == NULL ) {
      intf->rstate.payload_handler = intf->filter(intf);
      if( intf->rstate.payload_handler == NULL ) {
        intf->rstate.payload_handler = ppp_payload_handler_drop;
      }
    }

    // process payload
    for(;;) {
      int8_t phret = intf->rstate.payload_handler(intf);
      if( phret == -1 ) {
        return;
      } else if( phret == 0 ) {
        break;
      }
      // phret == 1
      intf->rstate.payload_handler = ppp_payload_handler_drop;
    }

    // reset state for the next frame
    *pos = 0;
    *crc = 0xffff;
    intf->rstate.udata = NULL;
    intf->rstate.payload_handler = NULL;
#if PPP_PAYLOAD_BUF_SIZE
    intf->rstate.payload_pos = 0;
#endif
  }
  // never reached
}


int ppp_recv_frame_data(ppp_intf_t *intf)
{
  int ret = uart_recv_nowait(intf->uart);
  if(ret != -1) {
    intf->rstate.crc = _crc_ccitt_update(intf->rstate.crc, ret);
  }
  return ret;
}

int8_t ppp_recv_frame_crc(ppp_intf_t *intf)
{
  if( intf->rstate.pos == PPP_HEADER_SIZE+2 ) {
    int v = uart_recv_nowait(intf->uart);
    if( v == -1 ) {
      return -1;
    }
    intf->rstate.crc ^= (v & 0xff);
    intf->rstate.pos++;
  }
  if( intf->rstate.pos == PPP_HEADER_SIZE+3 ) {
    int v = uart_recv_nowait(intf->uart);
    if( v == -1 ) {
      return -1;
    }
    intf->rstate.crc ^= (v & 0xff) << 8;
    intf->rstate.pos++;
  }
  return intf->rstate.crc == 0;
}


#if PPP_PAYLOAD_BUF_SIZE

bool ppp_recv_frame_payload(ppp_intf_t *intf, uint8_t n)
{
  uint8_t *pos = &intf->rstate.payload_pos;
  while( *pos < n ) {
    int v = ppp_recv_frame_data(intf);
    if( v == -1 ) {
      return false;
    }
    intf->rstate.payload[*pos] = v;
    (*pos)++;
  }
  return true;
}

int8_t ppp_recv_frame_payload_all(ppp_intf_t *intf)
{
  if( !ppp_recv_frame_payload(intf, intf->rstate.header.plsize) ) {
    return -1;
  }
  return ppp_recv_frame_crc(intf);
}

#endif


int8_t ppp_payload_handler_drop(ppp_intf_t *intf)
{
  // drop payload data
  while( intf->rstate.header.plsize > 0 ) {
    if( uart_recv_nowait(intf->uart) == -1 ) {
      return -1;
    }
    intf->rstate.header.plsize--;
  }

  // drop CRC
  while( intf->rstate.pos < PPP_HEADER_SIZE+4 ) {
    if( uart_recv_nowait(intf->uart) == -1 ) {
      return -1;
    }
    intf->rstate.pos++;
  }

  return 0;
}



void ppp_send_frame(ppp_intf_t *intf, const ppp_header_t *header, const void *data)
{
  const uint8_t *data_ = data;
  PPP_SEND_INTLVL_DISABLE() {
    ppp_send_frame_header(intf, header);

    // send data, update CRC
    // note: cannot use ppp_send_frame_data() for which size is limited to 255
    uint16_t i;
    for( i=0; i<header->plsize; i++ ) {
      uart_send(intf->uart, data_[i]);
      intf->wstate.crc = _crc_ccitt_update(intf->wstate.crc, data_[i]);
    }

    ppp_send_frame_crc(intf);
  }
}


void ppp_send_frame_header(ppp_intf_t *intf, const ppp_header_t *header)
{
  uart_send(intf->uart, PPP_START_BYTE);
  uint8_t data[] = {
    header->plsize & 0xff, (header->plsize >> 8) & 0xff,
    header->src, header->dst,
    header->pltype,
  };

  intf->wstate.crc = 0xffff;
  ppp_send_frame_data(intf, data, sizeof(data));
  ppp_send_frame_crc(intf);
  intf->wstate.crc = 0xffff;
}


void ppp_send_frame_data(ppp_intf_t *intf, const void *data, uint8_t size)
{
  const uint8_t *data_ = data;
  uint8_t i;
  for( i=0; i<size; i++ ) {
    uart_send(intf->uart, data_[i]);
    intf->wstate.crc = _crc_ccitt_update(intf->wstate.crc, data_[i]);
  }
}


void ppp_send_frame_crc(ppp_intf_t *intf)
{
  uint16_t crc = intf->wstate.crc;
  uart_send(intf->uart, crc & 0xff);
  uart_send(intf->uart, (crc >> 8) & 0xff);
}



//@endcond
