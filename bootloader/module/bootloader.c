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
    "movw r0, %5\n"  // set word data to r0:r1
    "sts %4, %C3\n"  // set RAMPZ
    "sts %0, %1\n"   // set NVM.CMD for load command
    "sts %6, %7\n"   // disable CCP protection
    "spm\n"          // execute SPM operation
    "clr r1\n"       // clear r1, GCC's __zero_reg__
    "sts %0, %8\n"   // clear NVM.CMD
    :
    : "i" (_SFR_MEM_ADDR(NVM_CMD)),
      "r" ((uint8_t)(NVM_CMD_LOAD_FLASH_BUFFER_gc)),
      "z" ((uint16_t)(address)),
      "r" ((uint32_t)(address)),
      "i" (_SFR_MEM_ADDR(RAMPZ)),
      "r" ((uint16_t)(word)),
      "i" (_SFR_MEM_ADDR(CCP)),
      "r" ((uint8_t)(CCP_SPM_gc)),
      "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
    : "r0"
  );
}


/// Erase then write a page
static void boot_app_page_erase_write(uint32_t address)
{
  asm volatile (
    "sts %4, %C3\n"  // set RAMPZ
    "sts %0, %1\n"   // set NVM.CMD for load command
    "sts %5, %6\n"   // disable CCP protection
    "spm\n"          // execute SPM operation
    "clr r1\n"       // clear r1, GCC's __zero_reg__
    "sts %0, %7\n"   // clear NVM.CMD
    :
    : "i" (_SFR_MEM_ADDR(NVM_CMD)),
      "r" ((uint8_t)(NVM_CMD_ERASE_WRITE_APP_PAGE_gc)),
      "z" ((uint16_t)(address)),
      "r" ((uint32_t)(address)),
      "i" (_SFR_MEM_ADDR(RAMPZ)),
      "i" (_SFR_MEM_ADDR(CCP)),
      "r" ((uint8_t)(CCP_SPM_gc)),
      "r" ((uint8_t)(NVM_CMD_NO_OPERATION_gc))
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


/// Disable watchdog
static void wdt_disable(void)
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

static bool uart_poll(void)
{
  return BOOTLOADER_UART.STATUS & USART_RXCIF_bm;
}

//@}


/** @name General protocol definitions
 */
//@{

#define STATUS_NONE             0x00
#define STATUS_SUCCESS          0x01
#define STATUS_BOOTLOADER_MSG   0x0a // LF
#define STATUS_FAILURE          0xff
#define STATUS_UNKNOWN_COMMAND  0x81
#define STATUS_BAD_VALUE        0x82
#define STATUS_CRC_MISMATCH     0x90


static void send_u8(uint8_t v)
{
  uart_send(v);
}

static void send_buf(const uint8_t *buf, uint8_t n)
{
  while(n--) {
    send_u8(*buf++);
  }
}

static void send_u16(uint16_t v)
{
  send_u8(v & 0xff);
  send_u8((v >> 8) & 0xff);
}

/// Send a NUL terminated string (without the NUL character)
static void send_str(const char *s)
{
  while(*s) {
    send_u8(*s++);
  }
}

static uint8_t recv_u8(void)
{
  return uart_recv();
}

/** @brief Send a human-readable reply
 *
 * This method is intended for messages sent by the bootloader which may not be
 * handled by a client (eg. enter/exit messages).
 *
 * \e msg must be 10 character long.
 * The resulting message will be \e msg surrounded by CRLF sequences, and still
 * be a valid protocol reply.
 */
#define SEND_MESSAGE(msg) send_str("\r\n" msg "\r\n")

static uint16_t recv_u16(void)
{
  uint8_t b[2];
  b[0] = recv_u8();
  b[1] = recv_u8();
  return b[0] + (b[1] << 8);
}

static uint32_t recv_u32(void)
{
  uint16_t w[2];
  w[0] = recv_u16();
  w[1] = recv_u16();
  uint32_t ret = w[0];
  ret += w[1] * 0x10000U;
  return ret;
}


/// Send a reply with given status and field size
static void reply(uint8_t st, uint8_t size)
{
  send_u8(size + 1);
  send_u8(st);
}

/// Prepare a success reply with a given field size
static void reply_success(uint8_t size)
{
  reply(STATUS_SUCCESS, size);
}

/// Send a failure reply with no fields
static void reply_failure(void)
{
  reply(STATUS_FAILURE, 0);
}

/// Send a custom error reply with no fields
static void reply_error(uint8_t st)
{
  reply(st, 0);
}

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
}


/** @name Protocol commands
 *
 * Commands format is:
 *  - a command (u8)
 *  - parameters
 *
 * Reply format is:
 *  - reply size (u8), not counting the size byte itself
 *  - a status code (u8)
 *  - reply fields
 *
 * @note The reply size is never 0 since it includes at least the status code.
 *
 * \e SZ designates a null-terminated string. Addresses are provided as u32.
 * All values are coded in little-endian.
 */
//@{

/** @brief Dump general infos
 *
 * Reply fields:
 *  - PROGMEM_PAGE_SIZE value (u16)
 */
static void cmd_infos(void)
{
  static const uint8_t infos_buf[] = {
    PROGMEM_PAGE_SIZE & 0xff, (PROGMEM_PAGE_SIZE >> 8) & 0xff,
  };
  reply_success(sizeof(infos_buf));
  send_buf(infos_buf, sizeof(infos_buf));
}


/** @brief Read a byte and send it back
 */
static void cmd_mirror(void)
{
  const uint8_t c = recv_u8();
  reply_success(1);
  send_u8(c);
}

/** @brief Terminate the connection and run the application
 *
 * The server replies to acknowledge, then resets.
 */
static void cmd_execute(void)
{
  reply_success(0);
  boot();
}


/** @brief Program a page
 *
 * When using CRC check, the page is now written on mismatch.
 *
 * Parameters:
 *  - page address (u32), must be aligned
 *  - CRC (u16)
 *  - page data
 */
static void cmd_prog_page(void)
{
  const uint32_t addr = recv_u32();
  // addr must be aligned on page size, and be in the bootloader area
  if(addr > APP_SECTION_END || addr % PROGMEM_PAGE_SIZE != 0) {
    reply_error(STATUS_BAD_VALUE);
    return;
  }

  const uint16_t crc_expected = recv_u16();
  uint16_t crc = 0xffff;

  // Read data, fill temporary page buffer, compute CRC
  for(uint16_t i=0; i<PROGMEM_PAGE_SIZE/2; i++) {
    uint8_t c1 = recv_u8();
    uint8_t c2 = recv_u8();
    crc = _crc_ccitt_update(crc, c1);
    crc = _crc_ccitt_update(crc, c2);
    uint16_t w = c1 + (c2 << 8); // little endian word
    boot_flash_page_fill(addr+2*i, w);
  }

  // check CRC
  if(crc != crc_expected) {
    reply_error(STATUS_CRC_MISMATCH);
    return;
  }

  // Erase and write the page
  boot_app_page_erase_write(addr);
  boot_nvm_busy_wait();

  reply_success(0);
}


/** @brief Compute the CRC of a given memory range
 *
 * The client asks for a CRC, providing an address and a size, and the server
 * replies with the computed CRC.
 *
 * Parameters:
 *  - start address (u32)
 *  - size (u32)
 *
 * Reply: computed CRC (u16)
 */
static void cmd_mem_crc(void)
{
  const uint32_t start = recv_u32();
  const uint32_t size = recv_u32();

  if(start + size > APP_SECTION_END+1) {
    reply_error(STATUS_BAD_VALUE);
    return;
  }

  // Compute CRC
  uint16_t crc = 0xffff;
  for(uint32_t addr=start; addr<start+size; addr++) {
    const uint8_t c = pgm_read_byte_bootloader(addr);
    crc = _crc_ccitt_update(crc, c);
  }

  reply_success(2);
  send_u16(crc);
}


/** @brief Dump fuse values
 *
 * Reply fields: fuses as u8s (6 bytes).
 */
static void cmd_fuse_read(void)
{
  reply_success(3);
  for(uint8_t i=0; i<FUSE_SIZE; i++) {
    send_u8(boot_lock_fuse_bits_get(i));
  }
}

//@}



int main(void) __attribute__ ((OS_main));
int main(void)
{
  // switch vector table (interrupts are not used anyway)
  ccp_io_write(&PMIC.CTRL, PMIC.CTRL | PMIC_IVSEL_bm);
  wdt_disable();

  clock_init();

#ifdef BOOTLOADER_INIT_CODE
  do{ BOOTLOADER_INIT_CODE }while(0);
#endif

  uart_init();

  SEND_MESSAGE("boot ENTER");
  // timeout before booting
  uint8_t i = (uint32_t)(BOOTLOADER_TIMEOUT) * (float)CLOCK_CPU_FREQ/(65536*4*1000);
  for(;;) {
    // detect activity from client
    if(uart_poll()) {
      break;
    }

    if(i == 0) {
      boot(); // timeout
    }
    i--;
    _delay_loop_2(0); // 65536*4 cycles
  }

#ifdef BOOTLOADER_ENTER_CODE
  do{ BOOTLOADER_ENTER_CODE }while(0);
#endif

  for(;;) {
    const uint8_t c = recv_u8();
    if(c == 0x00) {
      continue; // null command: ignore
    } else if(c == 0xff) {
      // failure command
      reply_failure();
    }
    else if(c == 'i') cmd_infos();
    else if(c == 'm') cmd_mirror();
    else if(c == 'x') cmd_execute();
    else if(c == 'p') cmd_prog_page();
    else if(c == 'c') cmd_mem_crc();
    else if(c == 'f') cmd_fuse_read();
    else {
      reply_error(STATUS_UNKNOWN_COMMAND);
    }
  }
}

//@}
