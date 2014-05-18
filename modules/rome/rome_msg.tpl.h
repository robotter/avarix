// Generation date: $$avarix:time.strftime('%Y-%m-%d %H:%m:%S')$$
/** @addtogroup ROME */
//@{
/** @file
 * @brief Definition of ROME frame and messages
 *
 * @note
 * This file is generated.
 * Generated symbols for a \e dummy message are provided.
 */
#ifndef ROME_MSG_H__
#define ROME_MSG_H__

#include <rome/rome.h>


#if DOXYGEN

/// ROME message IDs
typedef enum {
  ROME_MID_DUMMY = 0x42,
} rome_mid_t;

/// ROME frame
typedef struct {
  uint8_t plsize;  ///< length of payload data
  uint8_t mid; ///< message ID
  union {
    uint8_t _data[3];
    /// Data of dummy message
    struct {
      uint8_t a;
      int16_t b;
    } dummy;
  };
} __attribute__((__packed__)) rome_frame_t;

#else

typedef enum {
#pragma avarix_tpl self.mid_enum_fields()
} __attribute__((__packed__)) rome_mid_t;

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

//@}

#else

#pragma avarix_tpl self.macro_helpers()

#pragma avarix_tpl self.macro_disablers()

#endif

#endif
//@}
