/**
 * @cond internal
 * @file
 */
#include "../payloads.h"
#include "system.h"

#if PPP_PAYLOAD_BUF_SIZE < 1
# error PPP_PAYLOAD_BUF_SIZE must be at least 1 for system payload support
#endif

#define SYSTEM_ID_ACK  0
#define SYSTEM_ID_NAK  1
#define SYSTEM_ID_PING  2
#define SYSTEM_ID_TRACEROUTE  3
#define SYSTEM_ID_NAME  4
#define SYSTEM_ID_STOP  5
#define SYSTEM_ID_RESET  6
#define SYSTEM_ID_SUPPORTED_PAYLOADS  7

#define SYSTEM_REQUEST(id) (SYSTEM_ID_ ## id)
#define SYSTEM_RESPONSE(id) (SYSTEM_ID_ ## id | 0x80)


// Always support system payload
#define PPP_SUPPORT_SYSTEM


#define FOREACH_NIBBLE_VAL(e,x) \
    e(x,0) e(x,1) e(x,2) e(x,3) \
    e(x,4) e(x,5) e(x,6) e(x,7) \
    e(x,8) e(x,9) e(x,A) e(x,B) \
    e(x,C) e(x,D) e(x,E) e(x,F)

#define FOREACH_BYTE_VAL(e) \
    FOREACH_NIBBLE_VAL(e,0) FOREACH_NIBBLE_VAL(e,1) \
    FOREACH_NIBBLE_VAL(e,2) FOREACH_NIBBLE_VAL(e,3) \
    FOREACH_NIBBLE_VAL(e,4) FOREACH_NIBBLE_VAL(e,5) \
    FOREACH_NIBBLE_VAL(e,6) FOREACH_NIBBLE_VAL(e,7) \
    FOREACH_NIBBLE_VAL(e,8) FOREACH_NIBBLE_VAL(e,9) \
    FOREACH_NIBBLE_VAL(e,A) FOREACH_NIBBLE_VAL(e,B) \
    FOREACH_NIBBLE_VAL(e,C) FOREACH_NIBBLE_VAL(e,D) \
    FOREACH_NIBBLE_VAL(e,E) FOREACH_NIBBLE_VAL(e,F)

#define T(x) #x
#define TT(x) T(x)

// Used with FOREACH_BYTE_VAL to define a bitfield for each payload type
#define EXPR_BITFIELD(x,y)  uint8_t b_0x##x##y:1;

/* Used with PPP_TYPE_FOREACH to set supported payloads
 *
 * For each known payload type the associated bitfield is set if
 * PPP_SUPPORT_NAME is defined.
 * In such case TT(PPP_SUPPORT_##n) expands to an empty string. Then, using
 * sizeof(), we can emulate '#ifdef' without using preprocessor.
 *
 * If the macro is defined, TT(PPP_SUPPORT_##n) expands as:
 *   TT(PPP_SUPPORT_NAME) -> T() -> ""
 * If the macro is undefined, TT(PPP_SUPPORT_##n) expands as:
 *  TT(PPP_SUPPORT_NAME) -> T(PPP_SUPPORT_NAME) -> "PPP_SUPPORT_NAME"
 *
 * Undefined payload types default to 0, like any static.
 */
#define EXPR_SET_SUPPORTED(v,n) .b_ ## v = sizeof(TT(PPP_SUPPORT_##n)) == 1,


static const union {
  struct {
    uint8_t id; // first byte of supported payloads reply
    FOREACH_BYTE_VAL(EXPR_BITFIELD)
  } __attribute__((__packed__)) bits;
  uint8_t data[32+1];

} supported_payloads = { .bits = {
  .id = SYSTEM_RESPONSE(SUPPORTED_PAYLOADS),
  PPP_TYPE_FOREACH(EXPR_SET_SUPPORTED)
} };

#undef EXPR_SET_BIT
#undef EXPR_BITFIELD
#undef TT
#undef T
#undef FOREACHx
#undef FOREACH



/** @brief Send a request system payload
 *
 * \e data must be suitable for \e sizeof().
 */
#define SEND_SYSTEM_REQUEST(intf,_dst,data)  do { \
  const ppp_header_t header = { \
    .plsize = sizeof(data), \
    .src = intf->addr, \
    .dst = _dst, \
    .pltype = PPP_TYPE_SYSTEM, \
  }; \
  ppp_send_frame(intf, &header, data); \
} while(0)

/** @brief Send a reply to a system payload
 *
 * \e data must be suitable for \e sizeof().
 */
#define SEND_SYSTEM_RESPONSE(intf,data)  do { \
  const ppp_header_t header = { \
    .plsize = sizeof(data), \
    .src = intf->addr, \
    .dst = intf->rstate.header.src, \
    .pltype = PPP_TYPE_SYSTEM, \
  }; \
  ppp_send_frame(intf, &header, data); \
} while(0)


static int8_t payload_handler_ping(ppp_intf_t *intf)
{
  int8_t ret = ppp_recv_frame_payload_all(intf);
  if( ret == -1 ) {
    return -1;
  } else if( ret == 0 ) {
    return 0;
  }

  const uint8_t data[] = { SYSTEM_RESPONSE(PING), intf->rstate.payload[0] };
  SEND_SYSTEM_RESPONSE(intf, data);
  return 0;
}

static int8_t payload_handler_traceroute(ppp_intf_t *intf)
{
  int8_t ret = ppp_recv_frame_payload_all(intf);
  if( ret == -1 ) {
    return -1;
  } else if( ret == 0 ) {
    return 0;
  }

  const uint8_t data[] = { SYSTEM_RESPONSE(TRACEROUTE), 0 };
  SEND_SYSTEM_RESPONSE(intf, data);

  return 0;
}

static int8_t payload_handler_name(ppp_intf_t *intf)
{
  int8_t ret = ppp_recv_frame_payload_all(intf);
  if( ret == -1 ) {
    return -1;
  } else if( ret == 0 ) {
    return 0;
  }

  // cannot use SYSTEM_RESPONSE(NAME) here
  const uint8_t data[sizeof(PPP_NODE_NAME)] = "\x84" PPP_NODE_NAME;
  SEND_SYSTEM_RESPONSE(intf, data);

  return 0;
}

static int8_t payload_handler_supported_payloads(ppp_intf_t *intf)
{
  // skip data
  uint8_t *pos = &intf->rstate.payload_pos;
  while( *pos < intf->rstate.header.plsize ) {
    int v = uart_recv_nowait(intf->uart);
    if( v == -1 ) {
      return false;
    }
    intf->rstate.payload[*pos] = v;
    (*pos)++;
  }
  // check CRC
  int8_t ret = ppp_recv_frame_crc(intf);
  if( ret == -1 ) {
    return -1;
  } else if( ret == 0 ) {
    return 0;
  }

  SEND_SYSTEM_RESPONSE(intf, supported_payloads.data);

  return 0;
}


int8_t ppp_payload_handler_system(ppp_intf_t *intf)
{
  // receive the first byte
  int v = ppp_recv_frame_data(intf);
  if( v == -1 ) {
    return -1;
  }
  uint8_t id = v;

  ppp_header_t *const header = &intf->rstate.header;
  header->plsize--;

  // check payload size
  uint8_t expected_size;
  switch( id ) {
    case SYSTEM_REQUEST(STOP):
    case SYSTEM_REQUEST(RESET):
      expected_size = 0;
      break;
    case SYSTEM_REQUEST(PING):
    case SYSTEM_REQUEST(TRACEROUTE):
      expected_size = 1;
    case SYSTEM_REQUEST(NAME):
      expected_size = header->plsize;
      if(expected_size > 32) {
        return 1; // name is too long
      }
      break;
    case SYSTEM_REQUEST(SUPPORTED_PAYLOADS):
      expected_size = 32;
      break;
    case SYSTEM_RESPONSE(ACK):
    case SYSTEM_RESPONSE(NAK):
    case SYSTEM_RESPONSE(PING):
    case SYSTEM_RESPONSE(TRACEROUTE):
    case SYSTEM_RESPONSE(NAME):
    case SYSTEM_RESPONSE(STOP):
    case SYSTEM_RESPONSE(RESET):
    case SYSTEM_RESPONSE(SUPPORTED_PAYLOADS):
    default:
      return 1; // invalid or nothing to do: drop
  }
  if(header->plsize != expected_size) {
    return 1; // invalid size
  }

  // command-specific processing
  // if there are further data, replace the payload handler by a
  // command-specific one
  switch( id ) {
    case SYSTEM_REQUEST(PING):
      intf->rstate.payload_handler = payload_handler_ping;
      return payload_handler_ping(intf);
    case SYSTEM_REQUEST(TRACEROUTE):
      intf->rstate.payload_handler = payload_handler_traceroute;
      return payload_handler_traceroute(intf);
    case SYSTEM_REQUEST(NAME):
      intf->rstate.payload_handler = payload_handler_name;
      return payload_handler_name(intf);
    case SYSTEM_RESPONSE(SUPPORTED_PAYLOADS):
      intf->rstate.payload_handler = payload_handler_supported_payloads;
      return payload_handler_supported_payloads(intf);
    case SYSTEM_REQUEST(STOP):
    case SYSTEM_REQUEST(RESET): {
      //TODO do stop/reset
      const uint8_t data[] = { id | 0x80 };
      SEND_SYSTEM_RESPONSE(intf, data);
      return 0;
    }
  }
  return 1; // not reachable
}


void ppp_send_system_ack(ppp_intf_t *intf, bool ack)
{
  const uint8_t data[] = { ack ? SYSTEM_RESPONSE(ACK) : SYSTEM_RESPONSE(NAK) };
  SEND_SYSTEM_RESPONSE(intf, data);
}

void ppp_send_system_reset(ppp_intf_t *intf)
{
  const uint8_t data[] = { SYSTEM_REQUEST(RESET) };
  SEND_SYSTEM_REQUEST(intf, 0xFF, data);
}


///@endcond
