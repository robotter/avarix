/**
 * @file
 * @brief Content of device's user signature
 */
//@{
#ifndef AVARIX_SIGNATURE_H__
#define AVARIX_SIGNATURE_H__

#include <stdint.h>


/// Last user signature version
#define USER_SIGNATURE_VERSION  1


/** @brief Device identifier
 *
 * Identifier is a 4-character name which can also be manipulated as a 32-bit
 * integer (actual value is endianness-dependant though).
 */
typedef union {
  /// Device identifier name (string version)
  char name[4];
  /// Device identifier name (integer version)
  uint32_t fourcc;
} device_id_t;


/// Format a NUL-terminated string into a device identifier
#define DEVICE_ID_FROM_STR(id) ({ \
  _Static_assert(sizeof(id) == 5, "Invalid device ID name size"); \
  _Static_assert((id)[4] == '\0', "Unexpected device ID name terminator"); \
  (device_id_t){ .name = {id[0], id[1], id[2], id[3]} }; \
})


/// Content of user signature
typedef struct {
  /// Version of the user signature structure
  uint8_t version;
  /// Device identifier
  device_id_t id;
  /// Upload date of current program (UNIX timestamp)
  int32_t prog_date;
  /// Username who uploaded the current program
  char prog_username[32];

} user_sig_t;


/// Read the whole user signature
static inline void user_sig_read(user_sig_t *sig)
{
  register uint8_t cnt;
  asm volatile (
    "ldi %[cnt], %[size]\n"       // initialize data (down)counter
    "sts %[nvmcmd], %[cmdval]\n"  // set NVM.CMD for load command
    "loop:\n"                    
    "lpm r0, Z+\n"                // execute SPM operation
    "st Y+, r0\n"                 // write data to dest, increment dest
    "dec %[cnt]\n"                // update counter
    "brne loop\n"
    "sts %[nvmcmd], %[cmdnop]\n"  // clear NVM.CMD
    : [cnt]    "=r" (cnt)
    : [dest]   "y" (sig)
    , [size]   "M" (sizeof(*sig))
    , [nvmcmd] "i" (_SFR_MEM_ADDR(NVM_CMD))
    , [cmdval] "r" ((uint8_t)(NVM_CMD_READ_USER_SIG_ROW_gc))
    , [cmdnop] "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    , "z" ((uint16_t)0)
    : "r0"
  );
}


#endif
//@}
