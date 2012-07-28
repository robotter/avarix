// Generation date: $$avarix:time.strftime('%Y-%m-%d %H:%m:%S')$$
/** @addtogroup perlimpinpin */
//@{
/** @file
 * @brief Perlimpinpin ROOM payload support
 *
 * This file is generated.
 * Generated symbols for a \e dummy command are provided.
 */
#ifndef PERLIMPINPIN_PAYLOAD_ROOM_H__
#define PERLIMPINPIN_PAYLOAD_ROOM_H__

#include <perlimpinpin/perlimpinpin.h>


/// ROOM payload handler
int8_t ppp_payload_handler_room(ppp_intf_t *intf);


#if DOXYGEN

/// Message IDs
typedef enum {
  ROOM_MID_DUMMY = 0x42,
  ROOM_MID_DUMMY_R = 0x43,
} room_mid_t;

/// ROOM payload
typedef struct {
  room_mid_t mid; ///< message ID
  union {
    uint8_t _data[3];
    /// Data of dummy message
    struct {
      uint8_t a;
      int16_t b;
    } dummy;
    /// Data of dummy response message
    struct {
      uint8_t r;
    } dummy_r;
  };

} __attribute__((__packed__)) room_payload_t;

#else

typedef enum {
#pragma avarix_tpl self.mid_enum_fields()
} room_mid_t;

typedef struct {
  room_mid_t mid;
  union {
    uint8_t _data[$$avarix:self.max_param_size()$$];
#pragma avarix_tpl self.msgdata_union_fields()
  };
} __attribute__((__packed__)) room_payload_t;

#endif


/// Callback for received messages
typedef void room_message_handler_t(ppp_intf_t *, room_payload_t *);

/// Set handler for received messages
void room_set_message_handler(room_message_handler_t);


#if DOXYGEN

/** @name Helper macros to send messages
 *
 * These macros prepare and send a frame (header and data).
 */
//@{

/// Send a dummy message, generic version
#define ROOM_MSG_DUMMY(intf, dest, a, b)
/// Send a dummy_r message, generic version
#define ROOM_MSG_DUMMY_R(intf, dest, r)

/// Send a dummy request message (only for commands)
#define ROOM_SEND_DUMMY(intf, dest, a, b)

/// Send a response to a dummy request
#define ROOM_REPLY_DUMMY(intf, request, r) \

//@}

#else
#pragma avarix_tpl self.send_helpers()
#endif


#endif
//@}
