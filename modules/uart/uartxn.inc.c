/**
 * @internal
 * @file
 * @brief Include template code for uart.c
 *
 * The XN_(p,s) macro must be defined before including.
 * It is automatically undefined at the end of this file.
 */
#include <avr/interrupt.h>
#include <avarix/portpin.h>
#include <clock/defs.h>

#define UARTXN(s) XN_(UART,s)
#define uartXN(s) XN_(uart,s)
#define USARTXN(s) XN_(USART,s)
#define uartXN_ XN_(uart,_)

// Configuration checks
#if (UARTXN(_BSCALE) < -7) || (UARTXN(_BSCALE) > 7)
#error Invalid UARTxn_BSCALE value, must be between -7 and 7
#endif


/// FIFO buffer for received data
static uint8_t uartXN(_rxbuf)[UARTXN(_RX_BUF_SIZE)];
/// FIFO buffer for sent data
static uint8_t uartXN(_txbuf)[UARTXN(_TX_BUF_SIZE)];

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

  // set TXD value high
  portpin_dirset(&PORTPIN_TXDN(uartXN_.usart));
  portpin_outset(&PORTPIN_TXDN(uartXN_.usart));
  // enable RXC interrupts
  uartXN_.usart->CTRLA = (UART_INTLVL << USART_RXCINTLVL_gp);
  // async mode, no parity, 1 stop bit, 8 data bits
  uartXN_.usart->CTRLC = USART_CMODE_ASYNCHRONOUS_gc
      | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;

#define UART_BSEL \
      ((UARTXN(_BSCALE) >= 0) \
       ? (uint16_t)( (float)(CLOCK_CPU_FREQ) / ((1L<<UARTXN(_BSCALE)) * 16 * (unsigned long)UARTXN(_BAUDRATE)) - 1 ) \
       : (uint16_t)( (1L<<-UARTXN(_BSCALE)) * (((float)(CLOCK_CPU_FREQ) / (16 * (unsigned long)UARTXN(_BAUDRATE))) - 1) ) \
      )
  // baudrate, updated when BAUDCTRLA is written so set it after BAUDCTRLB
  uartXN_.usart->BAUDCTRLB = ((UART_BSEL >> 8) & 0x0F) | (UARTXN(_BSCALE) << USART_BSCALE_gp);
  uartXN_.usart->BAUDCTRLA = (UART_BSEL & 0xFF);
#undef UART_BSEL
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
