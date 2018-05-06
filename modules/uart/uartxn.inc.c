/**
 * @internal
 * @file
 * @brief Include template code for uart.c
 *
 * The XN_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <avr/interrupt.h>
#include <avarix/internal.h>
#include <avarix/portpin.h>
#include <clock/defs.h>

#define UARTXN(s) XN_(UART,s)
#define uartXN(s) XN_(uart,s)
#define USARTXN(s) XN_(USART,s)
#define uartXN_ XN_(uart,_)

// Configuration checks
#if (UARTXN(_BSCALE) < -6) || (UARTXN(_BSCALE) > 7)
// limit is -6 and not -7 for 10-bit frames
// (see "Fractional Baud Rate Generation" constraints in datasheet)
# error Invalid UARTxn_BSCALE value, must be between -6 and 7
#endif
#if UARTXN(_RX_BUF_SIZE) > 255
# error Invalid UARTxn_RX_BUF_SIZE value, max is 255
#endif
#if UARTXN(_TX_BUF_SIZE) > 255
# error Invalid UARTxn_TX_BUF_SIZE value, max is 255
#endif

#define UARTXN_BSEL \
    ((UARTXN(_BSCALE) >= 0) \
     ? (uint16_t)( 0.5 + (float)(CLOCK_CPU_FREQ) / ((1L<<UARTXN(_BSCALE)) * 16 * (unsigned long)UARTXN(_BAUDRATE)) - 1 ) \
     : (uint16_t)( 0.5 + (1L<<-UARTXN(_BSCALE)) * (((float)(CLOCK_CPU_FREQ) / (16 * (unsigned long)UARTXN(_BAUDRATE))) - 1) ) \
    )

// check difference between configured and actual bitrate
// error rate less than 1% should be possible for all usual baudrates
#define UART_ACTUAL_BAUDRATE \
    ((UARTXN(_BSCALE) >= 0) \
     ? ( (float)(CLOCK_CPU_FREQ) / ((1L<<UARTXN(_BSCALE)) * 16 * (UARTXN_BSEL + 1)) ) \
     : ( (1L<<-UARTXN(_BSCALE)) * (float)(CLOCK_CPU_FREQ) / (16 * (UARTXN_BSEL + (1L<<-UARTXN(_BSCALE)))) ) \
    )
_Static_assert(UART_ACTUAL_BAUDRATE/UARTXN(_BAUDRATE) < 1.01 &&
               UART_ACTUAL_BAUDRATE/UARTXN(_BAUDRATE) > 0.99,
               "Baudrate error is higher than 1%, try with another UARTxn_BSCALE value");
#undef UART_ACTUAL_BAUDRATE


/// FIFO buffer for received data
static uint8_t uartXN(_rxbuf)[UARTXN(_RX_BUF_SIZE)] AVARIX_DATA_NOINIT;
/// FIFO buffer for sent data
static uint8_t uartXN(_txbuf)[UARTXN(_TX_BUF_SIZE)] AVARIX_DATA_NOINIT;

static uart_t uartXN_ = {
  .usart = &USARTXN(),
  .rxbuf = { NULL, NULL, uartXN(_rxbuf), sizeof(uartXN(_rxbuf)) },
  .txbuf = { NULL, NULL, uartXN(_txbuf), sizeof(uartXN(_txbuf)) },
};

uart_t *const uartXN() = &uartXN_;


void uartXN(_init)(void)
{
  uart_buf_init(&uartXN_.rxbuf);
  uart_buf_init(&uartXN_.txbuf);

  // set TXD to output
  portpin_dirset(&PORTPIN_TXDN(uartXN_.usart));
  // enable RXC interrupts
  uartXN_.usart->CTRLA = (UART_INTLVL << USART_RXCINTLVL_gp);
  // async mode, no parity, 1 stop bit, 8 data bits
  uartXN_.usart->CTRLC = USART_CMODE_ASYNCHRONOUS_gc
      | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;

  // baudrate, updated when BAUDCTRLA is written so set it after BAUDCTRLB
  uartXN_.usart->BAUDCTRLB = ((UARTXN_BSEL >> 8) & 0x0F) | ((UARTXN(_BSCALE) << USART_BSCALE_gp) & USART_BSCALE_gm);
  uartXN_.usart->BAUDCTRLA = (UARTXN_BSEL & 0xFF);
  // enable RX, enable TX
  uartXN_.usart->CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}


/// Interrupt handler for received data
ISR(USARTXN(_RXC_vect))
{
  uint8_t v = uartXN_.usart->DATA;
  if( !uart_buf_full(&uartXN_.rxbuf) ) {
    uart_buf_push(&uartXN_.rxbuf, v);
  }
}

/// Interrupt handler for sent data
ISR(USARTXN(_DRE_vect))
{
  uart_send_buf_byte(&uartXN_);
}


#undef UARTXN
#undef uartXN
#undef USARTXN
#undef uartXN_
#undef XN_
#undef UARTXN_BSEL
