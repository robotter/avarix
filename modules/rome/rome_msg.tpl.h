// Generation date: $$avarix:time.strftime('%Y-%m-%d %H:%m:%S')$$
/** @addtogroup rome */
//@{
/** @file
 * @brief Definition of ROME frame and messages
 *
 * @note
 * This file is generated.
 * Generated symbols for a \e dummy message, a \e fake order and a \e switch
 * enum are provided.
 */
#ifndef ROME_MSG_H__
#define ROME_MSG_H__

// check ROME_ACK_MIN and ROME_ACK_MAX
#if (defined ROME_ACK_MIN) && !(defined ROME_ACK_MAX)
# error ROME_ACK_MIN defined but ROME_ACK_MAX is not
#elif (defined ROME_ACK_MAX) && !(defined ROME_ACK_MIN)
# error ROME_ACK_MAX defined but ROME_ACK_MIN is not
#elif (defined ROME_ACK_MIN) && (defined ROME_ACK_MAX)
# if ROME_ACK_MIN < 0 || ROME_ACK_MIN > 255
#  error ROME_ACK_MIN is out of range
# elif ROME_ACK_MAX < 0 || ROME_ACK_MAX > 255
#  error ROME_ACK_MAX is out of range
# elif ROME_ACK_MAX < ROME_ACK_MIN
#  error ROME_ACK_MAX must be greater or equal than ROME_ACK_MIN
# endif
#endif

// include after checks of min/max values, on purpose
#include <rome/rome.h>


#if DOXYGEN

/// ROME message IDs
typedef enum {
  ROME_MID_DUMMY = 0x42,
  ROME_MID_FAKE = 0x43,
} rome_mid_t;

typedef enum {
  ROME_ENUM_SWITCH_OFF = 0,
  ROME_ENUM_SWITCH_ON = 1,
} rome_enum_switch_t;

/// ROME frame
typedef struct {
  uint8_t plsize;  ///< length of payload data
  uint8_t mid; ///< message ID
  union {
    uint8_t _data[3];
    /// Data of dummy message
    struct {
      rome_enum_switch_t a;
      int16_t b;
    } dummy;
    /// Data of fake order
    struct {
      uint8_t _ack;
      uint16_t x;
    } fake;
  };
} __attribute__((__packed__)) rome_frame_t;

#else

typedef enum {
#pragma avarix_tpl self.mid_enum_fields()
} __attribute__((__packed__)) rome_mid_t;

#pragma avarix_tpl self.enum_types()

typedef struct {
  uint8_t plsize;
  uint8_t mid;
  union {
    uint8_t _data[$$avarix:self.max_param_size()$$];
#pragma avarix_tpl self.msgdata_union_fields()
  };
} __attribute__((__packed__)) rome_frame_t;

#endif


#if DOXYGEN

/** @name Helper macros to deal with frames
 */
//@{

/// Set data of a dummy message frame
#define ROME_SET_DUMMY(frame, a, b)

/// Send a dummy message
#define ROME_SEND_DUMMY(intf, a, b)

/// Set data of a fake order frame
#define ROME_SET_FAKE(frame, _ack, x)

/// Send a fake order
#define ROME_SEND_FAKE(intf, _ack, x)

/// Send a fake order until an ACK is received
#define ROME_SENDWAIT_FAKE(intf, x)

//@}

#else

#pragma avarix_tpl self.macro_helpers()

#pragma avarix_tpl self.macro_disablers()

#endif

#endif
//@}
