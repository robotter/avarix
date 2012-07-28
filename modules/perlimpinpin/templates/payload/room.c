/**
 * @cond internal
 * @file
 */
#include <perlimpinpin/payloads.h>
#include "room.h"


/// User-defined message handler
static room_message_handler_t *message_handler = NULL;

void room_set_message_handler(room_message_handler_t h)
{
  message_handler = h;
}


/// Second payload handler, for actual processing
static int8_t payload_handler_process(ppp_intf_t *intf)
{
  int8_t ret = ppp_recv_frame_payload_all(intf);
  if( ret == -1 ) {
    return -1;
  } else if( ret == 0 ) {
    return 0;
  }

  message_handler(intf, (room_payload_t*)intf->rstate.payload);
  return 0;
}


int8_t ppp_payload_handler_room(ppp_intf_t *intf)
{
  // receive the message type
  if( !ppp_recv_frame_payload(intf, 1) ) {
    return -1;
  }

  ppp_header_t *const header = &intf->rstate.header;

  // check payload size against mid
  uint8_t expected_size;
  switch( intf->rstate.payload[0] ) {
#pragma avarix_tpl self.check_payload_size_switch('expected_size')
    default:
      return 1; // invalid mid: drop
  }
  if(header->plsize-1 != expected_size) {
    return 1; // invalid size
  }

  // update handler to process payload and call it immediately
  intf->rstate.payload_handler = payload_handler_process;
  return payload_handler_process(intf);
}


//@endcond
