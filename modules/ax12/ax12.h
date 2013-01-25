/** @defgroup ax12 AX-12
 * @brief AX-12 servomotors module
 *
 * Except when otherwise stated, methods return an \ref ax12_error_t "error
 * code".
 */
//@{
/**
 * @file
 * @brief AX-12 module definitions
 */
#ifndef AX12_H__
#define AX12_H__

#include <stdint.h>
#include "address.h"
#include "ax12_config.h"


/// AX-12 instruction packet
typedef struct {
  uint8_t id; ///< AX-12 ID
  uint8_t instruction; ///< performed instruction (see \ref ax12_instruction_t)
  uint8_t nparams; ///< size of params array
  uint8_t params[AX12_MAX_PARAMS];  ///< parameter data
  uint8_t error; ///< error field, for status packet

} ax12_pkt_t;


/// Broadcast ID
#define AX12_BROADCAST_ID 0xFE


/// AX-12 instruction value
typedef enum {
  AX12_INSTR_PING = 0x01,
  AX12_INSTR_READ = 0x02,
  AX12_INSTR_WRITE = 0x03,
  AX12_INSTR_REG_WRITE = 0x04,
  AX12_INSTR_ACTION = 0x05,
  AX12_INSTR_RESET = 0x06,
  AX12_INSTR_SYNC_WRITE = 0x83,

} ax12_instruction_t;


/** @brief AX-12 error code
 *
 * The 7 lowest bits are AX-12 status flags.
 * When the MSB is set, value is a module-specific error code.
 */
typedef enum {
  AX12_ERROR_OK = 0,
  // error bits
  AX12_ERROR_BIT_VOLTAGE = (1 << 0),
  AX12_ERROR_BIT_ANGLE_LIMIT = (1 << 1),
  AX12_ERROR_BIT_OVERHEAT = (1 << 2),
  AX12_ERROR_BIT_RANGE = (1 << 3),
  AX12_ERROR_BIT_CHECKSUM = (1 << 4),
  AX12_ERROR_BIT_OVERLOAD = (1 << 5),
  AX12_ERROR_BIT_INSTRUCTION = (1 << 6),
  // custom error types (MSB set)
  AX12_ERROR_INVALID_PACKET = 0x80,
  AX12_ERROR_SEND_FAILED,
  AX12_ERROR_NO_REPLY,
  AX12_ERROR_REPLY_TIMEOUT,
  AX12_ERROR_BAD_CHECKSUM,

} ax12_error_t;


/// AX-12 UART line state
typedef enum {
  AX12_STATE_READ,
  AX12_STATE_WRITE,

} ax12_state_t;


/** @brief AX-12 connection interface
 *
 * Structure must be filled with valid callbacks before use.
 */
typedef struct {
  /** @brief Callback to send a byte
   *
   * Return 0 on success, -1 on error.
   */
  int8_t (*send)(uint8_t);
  /** @brief Callback to receive a byte
   *
   * Return the received byte or -1 on timeout.
   */
  int (*recv)(void);
  /// Switch UART line state
  void (*set_state)(ax12_state_t);

} ax12_t;



/// Send an AX-12 instruction packet
uint8_t ax12_send(ax12_t *s, const ax12_pkt_t *pkt);

/// Receive an AX-12 status packet
uint8_t ax12_recv(ax12_t *s, ax12_pkt_t *pkt);


/** @name Methods to access AX-12 memory
 */
//@{

/// Write a byte to AX-12 memory
uint8_t ax12_write_byte(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t data);

/// Write a word (2 bytes) to AX-12 memory
uint8_t ax12_write_word(ax12_t *s, uint8_t id, ax12_addr_t addr, uint16_t data);

/// Write n bytes to AX-12 memory
uint8_t ax12_write_mem(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t n, const uint8_t *data);

/// Read a byte from AX-12 memory
uint8_t ax12_read_byte(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t *data);

/// Read a word (2 bytes) from AX-12 memory
uint8_t ax12_read_word(ax12_t *s, uint8_t id, ax12_addr_t addr, uint16_t *data);

/// Read n bytes from AX-12 memory
uint8_t ax12_read_mem(ax12_t *s, uint8_t id, ax12_addr_t addr, uint8_t n, uint8_t *data);

//@}


/// Ping an AX-12, return the error register
uint8_t ax12_ping(ax12_t *s, uint8_t id);

/// Reset AX-12 back to factory settings
uint8_t ax12_reset(ax12_t *s, uint8_t id);


#endif
//@}
