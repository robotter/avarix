/**
 * @cond internal
 * @file
 */
#include <stdbool.h>
#include <avarix.h>
#include "uart.h"

// Configuration checks

#ifdef UARTxn_ENABLED
# warning UARTxn_ENABLED still defined, 'xn' should be replaced in configuration template
#endif

// Detect when at least one uart is enabled
#ifndef DOXYGEN
#define UART_EXPR(xn)  +1
#if (0 UART_ALL_APPLY_EXPR(UART_EXPR))
#define UART_HAS_UART_ENABLED
#endif
#undef UART_EXPR
#endif


/** @brief Circular FIFO buffer for UART data
 *
 * If head and tail are equal, the FIFO is empty.
 * The tail cannot catch up with the head by pushing; the last byte (before the
 * head) is always free.
 * This means the buffer is never completely filled. Its actual capacity is one
 * less than the buffer length.
 */
typedef struct {
  uint8_t *head;  ///< Pointer to the next byte to pop
  uint8_t *tail;  ///< Pointer to the next byte to push
  uint8_t *const data;  ///< Data buffer
  uint8_t len;  ///< Data buffer length
} uart_buf_t;



struct uart_struct {
  USART_t *const usart;  ///< Underlying USART structure
  uart_buf_t rxbuf;  ///< FIFO buffer for input data
  uart_buf_t txbuf;  ///< FIFO buffer for output data
};


/** @name FIFO buffers methods
 */
//@{

#ifdef UART_HAS_UART_ENABLED
/// Initialize a FIFO buffer
static void uart_buf_init(uart_buf_t *b)
{
  b->head = b->tail = b->data;
}
#endif

/// Get pointer to the end of a FIFO buffer
static uint8_t *uart_buf_end(const uart_buf_t *b)
{
  return b->data + b->len;
}

/// Check whether a FIFO buffer is full
static bool uart_buf_full(const uart_buf_t *b)
{
  return b->tail+1 == (b->head == b->data ? uart_buf_end(b) : b->head);
}

/// Check whether a FIFO buffer is empty
static bool uart_buf_empty(const uart_buf_t *b)
{
  return b->tail == b->head;
}

/// Push a byte to the FIFO buffer
static void uart_buf_push(uart_buf_t *b, uint8_t v)
{
  *b->tail = v;
  if( ++b->tail == uart_buf_end(b) ) {
    b->tail = b->data;
  }
}

/// Pop a byte from the FIFO buffer
static uint8_t uart_buf_pop(uart_buf_t *b)
{
  uint8_t v = *b->head;
  if( ++b->head == uart_buf_end(b) ) {
    b->head = b->data;
  }
  return v;
}

//@}


/** @brief Send the next waiting byte, if any
 * @note Must be called with global interrupt disabled
 */
static void uart_send_buf_byte(uart_t *u);


#define UART_EXPR(xn) \
    static void uart##xn##_init(void);
UART_ALL_APPLY_EXPR(UART_EXPR)
#undef UART_EXPR

void uart_init(void)
{
#define UART_EXPR(xn)  uart##xn##_init();
  UART_ALL_APPLY_EXPR(UART_EXPR)
#undef UART_EXPR
}


USART_t *uart_get_usart(uart_t *u)
{
  return u->usart;
}


int uart_recv(uart_t *u)
{
  int ret;
  while((ret = uart_recv_nowait(u)) < 0) ;
  return ret;
}

int uart_recv_nowait(uart_t *u)
{
  int ret;
  INTLVL_DISABLE_ALL_BLOCK() {
    if( uart_buf_empty(&u->rxbuf) ) {
      ret = -1;
    } else {
      ret = uart_buf_pop(&u->rxbuf);
    }
  }
  return ret;
}

int uart_send(uart_t *u, uint8_t v)
{
  while(uart_send_nowait(u, v) < 0) {
    if( !(CPU_SREG & CPU_I_bm) || !(PMIC.CTRL & INTLVL_BM(UART_INTLVL)) || (PMIC.STATUS & INTLVL_BM(UART_INTLVL)) ) {
      // UART interrupt disabled or blocked, avoid deadlock
      while( !(u->usart->STATUS & USART_DREIF_bm) ) ;
      // pop one byte from the buffer, should be the last iteration
      uart_send_buf_byte(u);
    }
  }
  return 0;
}

int uart_send_nowait(uart_t *u, uint8_t v)
{
  int ret;
  INTLVL_DISABLE_ALL_BLOCK() {
    if( uart_buf_full(&u->txbuf) ) {
      ret = -1;
    } else {
      uart_buf_push(&u->txbuf, v);
      u->usart->CTRLA |= (UART_INTLVL << USART_DREINTLVL_gp);
      ret = 0;
    }
  }
  return ret;
}

void uart_send_buf(uart_t *u, const uint8_t buf[], uint8_t len)
{
  for(uint8_t i=0; i<len; ++i) {
    uart_send(u, buf[i]);
  }
}

void uart_send_buf_byte(uart_t *u)
{
  if( uart_buf_empty(&u->txbuf) ) {
    u->usart->CTRLA &= ~USART_DREINTLVL_gm;
  } else {
    u->usart->DATA = uart_buf_pop(&u->txbuf);
    u->usart->CTRLA |= (UART_INTLVL << USART_DREINTLVL_gp);
  }
}



FILE *uart_fopen(uart_t *u)
{
  FILE *fp = fdevopen(uart_dev_send, uart_dev_recv);
  fdev_set_udata(fp, u);
  return fp;
}

int uart_dev_recv(FILE *fp)
{
  return uart_recv(fdev_get_udata(fp));
}

int uart_dev_send(char c, FILE *fp)
{
  return uart_send(fdev_get_udata(fp), c);
}



// Apply default configuration values
// Include template for each enabled UART

// Command to duplicate UARTC0 code for each UART.
// Vimmers will use it with " :*! ".
//   python -c 'import sys; s=sys.stdin.read(); print "\n".join(s.replace("C0",x+n) for x in "CDEF" for n in "01")'

#ifdef UARTC0_ENABLED
# define XN_(p,s)  p ## C0 ## s
# ifndef UARTC0_RX_BUF_SIZE
#  define UARTC0_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTC0_TX_BUF_SIZE
#  define UARTC0_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTC0_BAUDRATE
#  define UARTC0_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTC0_BSCALE
#  define UARTC0_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTC1_ENABLED
# define XN_(p,s)  p ## C1 ## s
# ifndef UARTC1_RX_BUF_SIZE
#  define UARTC1_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTC1_TX_BUF_SIZE
#  define UARTC1_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTC1_BAUDRATE
#  define UARTC1_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTC1_BSCALE
#  define UARTC1_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTD0_ENABLED
# define XN_(p,s)  p ## D0 ## s
# ifndef UARTD0_RX_BUF_SIZE
#  define UARTD0_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTD0_TX_BUF_SIZE
#  define UARTD0_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTD0_BAUDRATE
#  define UARTD0_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTD0_BSCALE
#  define UARTD0_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTD1_ENABLED
# define XN_(p,s)  p ## D1 ## s
# ifndef UARTD1_RX_BUF_SIZE
#  define UARTD1_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTD1_TX_BUF_SIZE
#  define UARTD1_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTD1_BAUDRATE
#  define UARTD1_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTD1_BSCALE
#  define UARTD1_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTE0_ENABLED
# define XN_(p,s)  p ## E0 ## s
# ifndef UARTE0_RX_BUF_SIZE
#  define UARTE0_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTE0_TX_BUF_SIZE
#  define UARTE0_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTE0_BAUDRATE
#  define UARTE0_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTE0_BSCALE
#  define UARTE0_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTE1_ENABLED
# define XN_(p,s)  p ## E1 ## s
# ifndef UARTE1_RX_BUF_SIZE
#  define UARTE1_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTE1_TX_BUF_SIZE
#  define UARTE1_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTE1_BAUDRATE
#  define UARTE1_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTE1_BSCALE
#  define UARTE1_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTF0_ENABLED
# define XN_(p,s)  p ## F0 ## s
# ifndef UARTF0_RX_BUF_SIZE
#  define UARTF0_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTF0_TX_BUF_SIZE
#  define UARTF0_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTF0_BAUDRATE
#  define UARTF0_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTF0_BSCALE
#  define UARTF0_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif

#ifdef UARTF1_ENABLED
# define XN_(p,s)  p ## F1 ## s
# ifndef UARTF1_RX_BUF_SIZE
#  define UARTF1_RX_BUF_SIZE  UART_RX_BUF_SIZE
# endif
# ifndef UARTF1_TX_BUF_SIZE
#  define UARTF1_TX_BUF_SIZE  UART_TX_BUF_SIZE
# endif
# ifndef UARTF1_BAUDRATE
#  define UARTF1_BAUDRATE  UART_BAUDRATE
# endif
# ifndef UARTF1_BSCALE
#  define UARTF1_BSCALE  UART_BSCALE
# endif
# include "uartxn.inc.c"
#endif


///@endcond
