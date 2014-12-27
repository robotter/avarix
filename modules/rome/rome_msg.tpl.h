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
#include <stdio.h>
#include <string.h>


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
 *
 * @note Helpers are not generated for variable-length messages.
 */
//@{

/** @brief Send a log message
 *
 * \e msg must be a literal string.
 */
#define ROME_LOG(intf, sev, msg)

/// Send a formatted log message
#define ROME_LOGF(intf, sev, fmt, ...)

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

/// Return maximum size of a variable-size field
#define ROME_MAX_FIELD_SIZE(msg, field)

//@}

#else

#define ROME_MAX_FIELD_SIZE(_msg, _field) \
   (sizeof((rome_frame_t*)0)->_data - (size_t)&((rome_frame_t*)0)->_msg._field)

#define ROME_LOG(_i, _sev, _msg) do { \
  uint8_t _buf[2 + 1 + sizeof(_msg)-1]; \
  rome_frame_t *_frame = (rome_frame_t*)_buf; \
  _frame->plsize = 1 + sizeof(_msg)-1; \
  _frame->mid = ROME_MID_LOG; \
  _frame->log.sev = ROME_ENUM_LOG_SEVERITY_##_sev; \
  memcpy(_frame->log.msg, _msg, sizeof(_msg)-1); \
  rome_send((_i), _frame); \
} while(0)

/// Send a formatted log message
#define ROME_LOGF(_i, _sev, _fmt, ...) do { \
  rome_frame_t _frame; \
  int n = snprintf(_frame.log.msg, ROME_MAX_FIELD_SIZE(log, msg)-1, _fmt, ##__VA_ARGS__); \
  _frame.plsize = 1 + (n <= (int)ROME_MAX_FIELD_SIZE(log, msg) ? n : (int)ROME_MAX_FIELD_SIZE(log, msg)); \
  _frame.mid = ROME_MID_LOG; \
  _frame.log.sev = ROME_ENUM_LOG_SEVERITY_##_sev; \
  rome_send((_i), &_frame); \
} while(0)

#pragma avarix_tpl self.macro_helpers()

#pragma avarix_tpl self.macro_disablers()

#endif

#endif
//@}
