/** @defgroup bootloader Bootloader
 * @brief Bootloader module
 *
 * The bootloader module is not a real module. It is not intended to be put as
 * dependency. Special build rules are available to builde the bootloader for a
 * project.
 *
 * The device which runs the bootloader will be referred as the \e server. The
 * remote client will be referred as the \e client.
 *
 * All CRC computations use the CRC-16-CCITT.
 */
//@{
/** @file
 *  @brief Bootloader code
 */
#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#include <clock/clock.h>
#include <util/delay.h>
#include <avarix/portpin.h>
#include <avarix/register.h>
#include <avarix/signature.h>
#include "bootloader_config.h"


#if defined(RAMPZ)
# define pgm_read_byte_bootloader  pgm_read_byte_far
#else
# define pgm_read_byte_bootloader  pgm_read_byte_near
#endif


/// Wait completion of NVM command
#define boot_nvm_busy_wait()          do{}while(NVM_STATUS & NVM_NVMBUSY_bm)


/// Load a word into page buffer
static void boot_flash_page_fill(uint32_t address, uint16_t word)
{
  asm volatile (
    "movw r0, %[word]\n"          // set word data to r0:r1
    "sts %[rampz], %C[addr32]\n"  // set RAMPZ
    "sts %[nvmcmd], %[cmdval]\n"  // set NVM.CMD for load command
    "sts %[ccp], %[ccpspm]\n"     // disable CCP protection
    "spm\n"                       // execute SPM operation
    "clr r1\n"                    // clear r1, GCC's __zero_reg__
    "sts %[nvmcmd], %[cmdnop]\n"  // clear NVM.CMD
    :
    : [nvmcmd] "i" (_SFR_MEM_ADDR(NVM_CMD))
    , [cmdval] "r" ((uint8_t)(NVM_CMD_LOAD_FLASH_BUFFER_gc))
    , [cmdnop] "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    , [ccp]    "i" (_SFR_MEM_ADDR(CCP))
    , [ccpspm] "r" ((uint8_t)(CCP_SPM_gc))
    ,          "z" ((uint16_t)(address))
    , [addr32] "r" ((uint32_t)(address))
    , [rampz]  "i" (_SFR_MEM_ADDR(RAMPZ))
    , [word]   "r" ((uint16_t)(word))
    : "r0", "r1"
  );
}


/// Erase then write a page
static void boot_app_page_erase_write(uint32_t address)
{
  asm volatile (
    "sts %[rampz], %C[addr32]\n"  // set RAMPZ
    "sts %[nvmcmd], %[cmdval]\n"  // set NVM.CMD for load command
    "sts %[ccp], %[ccpspm]\n"     // disable CCP protection
    "spm\n"                       // execute SPM operation
    "sts %[nvmcmd], %[cmdnop]\n"  // clear NVM.CMD
    :
    : [nvmcmd] "i" (_SFR_MEM_ADDR(NVM_CMD))
    , [cmdval] "r" ((uint8_t)(NVM_CMD_ERASE_WRITE_APP_PAGE_gc))
    , [cmdnop] "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    , [ccp]    "i" (_SFR_MEM_ADDR(CCP))
    , [ccpspm] "r" ((uint8_t)(CCP_SPM_gc))
    ,          "z" ((uint16_t)(address))
    , [addr32] "r" ((uint32_t)(address))
    , [rampz]  "i" (_SFR_MEM_ADDR(RAMPZ))
  );
}


/// Erase user signature page
static void boot_user_sig_erase(void)
{
  asm volatile (
    "sts %[nvmcmd], %[cmdval]\n"  // set NVM.CMD for load command
    "sts %[ccp], %[ccpspm]\n"     // disable CCP protection
    "spm\n"                       // execute SPM operation
    "sts %[nvmcmd], %[cmdnop]\n"  // clear NVM.CMD
    :
    : [nvmcmd] "i" (_SFR_MEM_ADDR(NVM_CMD))
    , [cmdval] "r" ((uint8_t)(NVM_CMD_ERASE_USER_SIG_ROW_gc))
    , [cmdnop] "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    , [ccp]    "i" (_SFR_MEM_ADDR(CCP))
    , [ccpspm] "r" ((uint8_t)(CCP_SPM_gc))
  );
}


/// Write user signature page
static void boot_user_sig_write(void)
{
  asm volatile (
    "sts %[nvmcmd], %[cmdval]\n"  // set NVM.CMD for load command
    "sts %[ccp], %[ccpspm]\n"     // disable CCP protection
    "spm\n"                       // execute SPM operation
    "sts %[nvmcmd], %[cmdnop]\n"  // clear NVM.CMD
    :
    : [nvmcmd] "i" (_SFR_MEM_ADDR(NVM_CMD))
    , [cmdval] "r" ((uint8_t)(NVM_CMD_WRITE_USER_SIG_ROW_gc))
    , [cmdnop] "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    , [ccp]    "i" (_SFR_MEM_ADDR(CCP))
    , [ccpspm] "r" ((uint8_t)(CCP_SPM_gc))
  );
}


static uint8_t boot_lock_fuse_bits_get(uint8_t offset)
{
  if(offset >= FUSE_SIZE) {
    return 0;
  }
  NVM.ADDR0 = offset;
  NVM.ADDR1 = 0;
  NVM.ADDR2 = 0;
  NVM.CMD = NVM_CMD_READ_FUSES_gc;
  ccp_io_write(&NVM.CTRLA, NVM_CMDEX_bm);
  boot_nvm_busy_wait();
  NVM.CMD = NVM_CMD_NO_OPERATION_gc;
  return NVM.DATA0;
}


//XXX wdt_disable() is provided by avr-libc but fails in 1.8.1
/// Disable watchdog
static void wdt_disable_(void)
{
  ccp_io_write(&WDT.CTRL, (WDT.CTRL & ~WDT_ENABLE_bm) | WDT_CEN_bm);
}


/** @brief Run the application
 * @note Registers are not initialized.
 */
static void run_app(void)
{
  // switch vector table
  ccp_io_write(&PMIC.CTRL, PMIC.CTRL & ~PMIC_IVSEL_bm);
  asm volatile (
    "ldi r30, 0\n"
    "ldi r31, 0\n"
    "ijmp\n"
  );
  __builtin_unreachable();
}


/** @name UART functions and configuration
 *
 * Configuration definitions are compatible with \e uart_config.h fields.
 * Parts of code are copied from the UART module.
 */
//@{

#ifndef BOOTLOADER_UART
# error BOOTLOADER_UART is not defined
#endif

#ifndef BOOTLOADER_UART_BAUDRATE
# ifdef UART_BAUDRATE
#  define BOOTLOADER_UART_BAUDRATE  UART_BAUDRATE
# else
#  error Neither BOOTLOADER_UART_BAUDRATE nor UART_BAUDRATE is defined
# endif
#endif

#ifndef BOOTLOADER_UART_BSCALE
# ifdef UART_BSCALE
#  define BOOTLOADER_UART_BSCALE  UART_BSCALE
# else
#  error Neither BOOTLOADER_UART_BSCALE nor UART_BSCALE is defined
# endif
#endif

// Configuration checks
#if (BOOTLOADER_UART_BSCALE < -6) || (BOOTLOADER_UART_BSCALE > 7)
# error Invalid BOOTLOADER_UART_BSCALE value, must be between -6 and 7
#endif

#define BOOTLOADER_UART_BSEL \
    ((BOOTLOADER_UART_BSCALE >= 0) \
     ? (uint16_t)( 0.5 + (float)(CLOCK_CPU_FREQ) / ((1L<<BOOTLOADER_UART_BSCALE) * 16 * (unsigned long)BOOTLOADER_UART_BAUDRATE) - 1 ) \
     : (uint16_t)( 0.5 + (1L<<-BOOTLOADER_UART_BSCALE) * (((float)(CLOCK_CPU_FREQ) / (16 * (unsigned long)BOOTLOADER_UART_BAUDRATE)) - 1) ) \
    )

// check difference between configured and actual bitrate
// error rate less than 1% should be possible for all usual baudrates
#define UART_ACTUAL_BAUDRATE \
    ((BOOTLOADER_UART_BSCALE >= 0) \
     ? ( (float)(CLOCK_CPU_FREQ) / ((1L<<BOOTLOADER_UART_BSCALE) * 16 * (BOOTLOADER_UART_BSEL + 1)) ) \
     : ( (1L<<-BOOTLOADER_UART_BSCALE) * (float)(CLOCK_CPU_FREQ) / (16 * (BOOTLOADER_UART_BSEL + (1L<<-BOOTLOADER_UART_BSCALE))) ) \
    )
_Static_assert(UART_ACTUAL_BAUDRATE/BOOTLOADER_UART_BAUDRATE < 1.01 &&
               UART_ACTUAL_BAUDRATE/BOOTLOADER_UART_BAUDRATE > 0.99,
               "Baudrate error is higher than 1%, try with another BOOTLOADER_UART_BSCALE value");
#undef UART_ACTUAL_BAUDRATE

static void uart_init(void)
{
  BOOTLOADER_UART.CTRLC = USART_CMODE_ASYNCHRONOUS_gc
      | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;

  // set TXD to output
  portpin_dirset(&PORTPIN_TXDN(&BOOTLOADER_UART));
  // baudrate, updated when BAUDCTRLA is written so set it after BAUDCTRLB
  BOOTLOADER_UART.BAUDCTRLB = ((BOOTLOADER_UART_BSEL >> 8) & 0x0F) | ((BOOTLOADER_UART_BSCALE << USART_BSCALE_gp) & USART_BSCALE_gm);
  BOOTLOADER_UART.BAUDCTRLA = (BOOTLOADER_UART_BSEL & 0xFF);
#undef BOOTLOADER_UART_BSEL
  // enable RX, enable TX
  BOOTLOADER_UART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}


static void uart_deinit(void)
{
  // wait for the last byte
  while(!(BOOTLOADER_UART.STATUS & (USART_DREIF_bm|USART_TXCIF_bm))) ;
  BOOTLOADER_UART.CTRLB = 0;  // disable
}

static void uart_send(char c)
{
  while(!(BOOTLOADER_UART.STATUS & USART_DREIF_bm)) ;
  BOOTLOADER_UART.DATA = c;
}

static char uart_recv(void)
{
  while(!(BOOTLOADER_UART.STATUS & USART_RXCIF_bm)) ;
  return BOOTLOADER_UART.DATA;
}

//@}


/** @name General protocol definitions
 */
//@{

#define ROME_START_BYTE  0x52 // 'R'
#define ROME_MID_BOOTLODADER    0x03
#define ROME_MID_BOOTLODADER_R  0x04

typedef enum {
  CMD_NONE = 0,
  CMD_BOOT,
  CMD_INFO,
  CMD_R_INFO,
  CMD_PROG_PAGE,
  CMD_R_PROG_PAGE,
  CMD_MEM_CRC,
  CMD_R_MEM_CRC,
  CMD_FUSE_READ,
  CMD_R_FUSE_READ,
  CMD_READ_USER_SIG,
  CMD_R_READ_USER_SIG,
  CMD_PROG_USER_SIG,
  CMD_R_PROG_USER_SIG,
} cmd_t;

typedef enum {
  STATUS_SUCCESS = 0,
  STATUS_ERROR,
  STATUS_UNKNOWN_CMD,
  STATUS_BAD_VALUE,
  STATUS_CRC_MISMATCH,
} status_t;


/// Bootloader message frame (ROME frame payload)
typedef struct {
  uint8_t ack;
  uint8_t cmd;
  uint8_t data[255-2];
} frame_t;


/** @brief Send a ROME bootloader reply frame
 *
 * \e size is the size of the bootloader frame data.
 */
static void send_rome_reply(uint8_t ack, uint8_t status, uint8_t *data, uint8_t size)
{
  uint16_t crc = 0xffff;

  // start byte, mid, payload size
  uart_send(ROME_START_BYTE);
  uint8_t plsize = size + 2;
  uart_send(plsize);
  crc = _crc_ccitt_update(crc, plsize);
  uart_send(ROME_MID_BOOTLODADER_R);
  crc = _crc_ccitt_update(crc, ROME_MID_BOOTLODADER_R);

  // frame data (ROME payload)
  uart_send(ack);
  crc = _crc_ccitt_update(crc, ack);
  uart_send(status);
  crc = _crc_ccitt_update(crc, status);
  for(uint8_t i=0; i<size; ++i) {
    uart_send(data[i]);
    crc = _crc_ccitt_update(crc, data[i]);
  }

  // CRC
  uart_send(crc & 0xff);
  uart_send((crc >> 8) & 0xff);
}

/// Send a status reply with no data
#define reply_status(frame, status)  send_rome_reply((frame)->ack, STATUS_##status, 0, 0)
/// Send a success reply with no data
#define reply_success(frame)  send_rome_reply((frame)->ack, STATUS_SUCCESS, 0, 0)
/// Send a success reply with data
#define reply_data(frame, data, size)  send_rome_reply((frame)->ack, STATUS_SUCCESS, (data), (size))

//@}


/** @brief Run the application
 */
static void boot(void)
{
  // extra null bytes to make sure the status is properly sent
  uart_send(0);
  uart_send(0);
  uart_deinit();

#ifdef BOOTLOADER_BOOT_CODE
  do{ BOOTLOADER_BOOT_CODE }while(0);
#endif
  run_app();
  __builtin_unreachable();
}


/** @name Bootloader commands
 */
//@{


/** @brief Terminate the connection and run the application
 *
 * The server replies to acknowledge, then resets.
 */
static void cmd_boot(const frame_t *frame)
{
  reply_success(frame);
  boot();
}


/** @brief Dump general info
 *
 * Reply data:
 *  - PROGMEM_PAGE_SIZE (uint16)
 */
static void cmd_info(const frame_t *frame)
{
  uint16_t data = PROGMEM_PAGE_SIZE;
  reply_data(frame, (void*)&data, sizeof(data));
}


/** @brief Program a page
 *
 * The page is not written on CRC mismatch.
 *
 * Command data:
 *  - page address (uint32), must be aligned
 *  - CRC (uint16)
 *
 * Read data:
 *  - page data (size: PROGMEM_PAGE_SIZE)
 */
static void cmd_prog_page(const frame_t *frame)
{
  struct {
    uint32_t addr;
    uint16_t crc;
  } *data = (void*)frame->data;
  uint32_t addr = data->addr;

  // addr must be aligned on page size, and be in the bootloader area
  if(addr > APP_SECTION_END || addr % PROGMEM_PAGE_SIZE != 0) {
    return reply_status(frame, BAD_VALUE);
  }

  // Read data, fill temporary page buffer, compute CRC
  uint16_t crc = 0xffff;
  for(uint16_t i=0; i<PROGMEM_PAGE_SIZE/2; i++) {
    uint8_t c1 = uart_recv();
    uint8_t c2 = uart_recv();
    crc = _crc_ccitt_update(crc, c1);
    crc = _crc_ccitt_update(crc, c2);
    uint16_t w = c1 + (c2 << 8); // little endian word
    boot_flash_page_fill(addr+2*i, w);
  }

  // check CRC
  if(crc != data->crc) {
    return reply_status(frame, CRC_MISMATCH);
  }

  // Erase and write the page
  boot_app_page_erase_write(addr);
  boot_nvm_busy_wait();

  reply_success(frame);
}


/** @brief Compute the CRC of a given memory range
 *
 * The client asks for a CRC, providing an address and a size, and the server
 * replies with the computed CRC.
 *
 * Command data:
 *  - start address (uint32)
 *  - size (uint32)
 *
 * Reply data:
 *  - computed CRC (uint16)
 */
static void cmd_mem_crc(const frame_t *frame)
{
  struct {
    uint32_t start;
    uint32_t size;
  } *data = (void*)frame->data;
  uint32_t start = data->start;
  uint32_t size = data->size;

  if(start + size > APP_SECTION_END+1) {
    return reply_status(frame, BAD_VALUE);
  }

  // Compute CRC
  uint16_t crc = 0xffff;
  for(uint32_t addr=start; addr<start+size; addr++) {
    const uint8_t c = pgm_read_byte_bootloader(addr);
    crc = _crc_ccitt_update(crc, c);
  }

  reply_data(frame, (void*)&crc, sizeof(crc));
}


/** @brief Read the user signature
 *
 * Reply data:
 *  - signature data (variable size)
 */
static void cmd_read_user_sig(const frame_t *frame)
{
  user_sig_t sig;
  user_sig_read(&sig);

  reply_data(frame, (void*)&sig, sizeof(sig));
}


/** @brief Program the user signature page
 *
 * The page is not written on CRC mismatch.
 *
 * Command data:
 *  - CRC (uint16)
 *  - data size (uint8)
 *
 * Read data:
 *  - data (size given in parameters)
 */
static void cmd_prog_user_sig(const frame_t *frame)
{
  struct {
    uint16_t crc;
    uint8_t data_size;
  } *data = (void*)frame->data;


  // Read data, fill temporary page buffer, compute CRC
  uint16_t crc = 0xffff;
  const uint8_t size = data->data_size;;
  for(uint8_t i=0; i<size; i++) {
    uint8_t c1 = uart_recv();
    crc = _crc_ccitt_update(crc, c1);
    i++;
    uint8_t c2;
    if(i < size) {
      c2 = uart_recv();
      crc = _crc_ccitt_update(crc, c2);
    } else {
      c2 = 0;
    }
    uint16_t w = c1 + (c2 << 8); // little endian word
    // note: this sets RAMPZ even if it's not necessary
    boot_flash_page_fill(i, w);
  }

  // check CRC
  if(crc != data->crc) {
    return reply_status(frame, CRC_MISMATCH);
  }

  // Erase and write the page
  boot_user_sig_erase();
  boot_nvm_busy_wait();
  boot_user_sig_write();
  boot_nvm_busy_wait();

  reply_success(frame);
}


/** @brief Dump fuse values
 *
 * Reply data:
 *  - fuses (uint8 array, 6 bytes)
 */
static void cmd_fuse_read(const frame_t *frame)
{
  uint8_t data[FUSE_SIZE];
  for(uint8_t i=0; i<FUSE_SIZE; i++) {
    data[i] = boot_lock_fuse_bits_get(i);
  }
  reply_data(frame, data, sizeof(data));
}


//@}


/// Receive on UART while waiting for timeout
static char uart_recv_timeout(uint32_t *timeout)
{
  // always decrease timeout, to timeout even when flooded
  while((*timeout)-- != 0) {
    if(BOOTLOADER_UART.STATUS & USART_RXCIF_bm) {
      return BOOTLOADER_UART.DATA;
    }
  }
  boot();
  __builtin_unreachable();
}


int main(void) __attribute__ ((OS_main));
int main(void)
{
  // switch vector table (interrupts are not used anyway)
  ccp_io_write(&PMIC.CTRL, PMIC.CTRL | PMIC_IVSEL_bm);
  wdt_disable_();

  clock_init();

#ifdef BOOTLOADER_INIT_CODE
  do{ BOOTLOADER_INIT_CODE }while(0);
#endif

  uart_init();

  // send a log message to signal we enter bootloader (precomputed)
  // retrieved with: rome.Frame('log', 'info', 'enter bootloader').data()
  static const char start_log[] = "R\x11\x02\1enter bootloader\xcd\xa0";
  for(uint8_t ii=0; ii<sizeof(start_log)-1; ii++) {
    uart_send(start_log[ii]);
  }

  // timeout before booting (approximate)
  const uint32_t timeout0 = (uint32_t)(BOOTLOADER_TIMEOUT) * (float)CLOCK_CPU_FREQ/(32.*1000);
  uint32_t timeout = timeout0;
  for(;;) {
    // start byte
    while(uart_recv_timeout(&timeout) != ROME_START_BYTE) ;

    uint16_t crc = 0xffff;

    // payload size, message ID
    uint8_t plsize = uart_recv_timeout(&timeout);
    uint8_t mid = uart_recv_timeout(&timeout);
    crc = _crc_ccitt_update(crc, plsize);
    crc = _crc_ccitt_update(crc, mid);

    // not a bootloader frame, or invalid payload (too small)
    if(mid != ROME_MID_BOOTLODADER || plsize < 1) {
      // consume payload but don't process
      for(uint8_t i=0; i<plsize; i++) {
        uart_recv_timeout(&timeout);
      }
      continue;
    }

    // payload data
    union {
      uint8_t buf[255];
      frame_t frame;
    } payload;
    for(uint8_t i=0; i<plsize; ++i) {
      char c = payload.buf[i] = uart_recv_timeout(&timeout);
      crc = _crc_ccitt_update(crc, c);
    }

    // CRC
    crc ^= uart_recv_timeout(&timeout);
    crc ^= uart_recv_timeout(&timeout) << 8;
    if(crc != 0) {
      continue;
    }

    // frame is valid, process it
    switch(payload.frame.cmd) {
      case CMD_BOOT: cmd_boot(&payload.frame); break;
      case CMD_INFO: cmd_info(&payload.frame); break;
      case CMD_PROG_PAGE: cmd_prog_page(&payload.frame); break;
      case CMD_MEM_CRC: cmd_mem_crc(&payload.frame); break;
      case CMD_FUSE_READ: cmd_fuse_read(&payload.frame); break;
      case CMD_READ_USER_SIG: cmd_read_user_sig(&payload.frame); break;
      case CMD_PROG_USER_SIG: cmd_prog_user_sig(&payload.frame); break;
      default: break;
    }
    timeout = timeout0;
  }
}

//@}
