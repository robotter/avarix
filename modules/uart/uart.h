/** @defgroup uart UART
 * @brief UART module
 *
 * @note
 * In this module documentation \e xn is an USART name composed of an
 * upper-case letter and a digit, for instance \c C0.
 *
 * First, UARTs must be initialized by calling uart_init().
 * Then, enabled UARTxn can be manipulated through \ref uartxn.
 *
 * The underlying USART structure can be accessed using \ref uart_get_usart(),
 * allowing to read or modify USART registers when advanced usage is required.
 *
 *
 * @par Standard I/O support
 *
 * UART can be used with standard I/O facilities.
 * The underlying \ref uart_t is stored as the stream's user data (\e udata
 * field).
 *
 * \ref uart_fopen() can be called to use an UART for operations on standard
 * streams.
 *
 * @sa \reflibc{group__avr__stdio.html,Standard I/O facilities} in avr-libc documentation.
 */
//@{
/**
 * @file
 * @brief UART definitions
 */
#ifndef UART_H__
#define UART_H__

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include "uart_config.h"


/// UART state
typedef struct uart_struct uart_t;


#ifdef DOXYGEN

/// State of UARTxn
extern uart_t *const uartxn;

#else

#ifdef UARTC0_ENABLED
# define UARTC0_APPLY_EXPR(f)  f(C0)
#else
# define UARTC0_APPLY_EXPR(f)
#endif
#ifdef UARTC1_ENABLED
# define UARTC1_APPLY_EXPR(f)  f(C1)
#else
# define UARTC1_APPLY_EXPR(f)
#endif
#ifdef UARTD0_ENABLED
# define UARTD0_APPLY_EXPR(f)  f(D0)
#else
# define UARTD0_APPLY_EXPR(f)
#endif
#ifdef UARTD1_ENABLED
# define UARTD1_APPLY_EXPR(f)  f(D1)
#else
# define UARTD1_APPLY_EXPR(f)
#endif
#ifdef UARTE0_ENABLED
# define UARTE0_APPLY_EXPR(f)  f(E0)
#else
# define UARTE0_APPLY_EXPR(f)
#endif
#ifdef UARTE1_ENABLED
# define UARTE1_APPLY_EXPR(f)  f(E1)
#else
# define UARTE1_APPLY_EXPR(f)
#endif
#ifdef UARTF0_ENABLED
# define UARTF0_APPLY_EXPR(f)  f(F0)
#else
# define UARTF0_APPLY_EXPR(f)
#endif
#ifdef UARTF1_ENABLED
# define UARTF1_APPLY_EXPR(f)  f(F1)
#else
# define UARTF1_APPLY_EXPR(f)
#endif

// Apply a macro to all enabled UARTs
#define UART_ALL_APPLY_EXPR(f)  \
  UARTC0_APPLY_EXPR(f) \
  UARTC1_APPLY_EXPR(f) \
  UARTD0_APPLY_EXPR(f) \
  UARTD1_APPLY_EXPR(f) \
  UARTE0_APPLY_EXPR(f) \
  UARTE1_APPLY_EXPR(f) \
  UARTF0_APPLY_EXPR(f) \
  UARTF1_APPLY_EXPR(f) \


#define UART_EXPR(xn) \
    extern uart_t *const uart##xn;
UART_ALL_APPLY_EXPR(UART_EXPR)
#undef UART_EXPR

#endif


/** @brief Initialize all enabled UARTs
 */
void uart_init(void);

/** @brief Get underlying USART structure
 */
USART_t *uart_get_usart(uart_t *u);

/** @brief Receive a single byte
 * @return The received value.
 */
int uart_recv(uart_t *u);

/** @brief Receive a single byte without blocking
 * @return The received value or -1 if it would block.
 */
int uart_recv_nowait(uart_t *u);

/** @brief Send a single byte
 * @return Always 0.
 */
int uart_send(uart_t *u, uint8_t v);

/** @brief Send a single byte without blocking
 * @return 0 if data has been sent, -1 otherwise.
 */
int uart_send_nowait(uart_t *u, uint8_t v);


/** @brief Open an UART as a standard stream
 *
 * Calls \c fdevopen() and associates the returned stream to the given UART.
 *
 * Since the first stream opened with \c fdevopen() is assigned to \c stdin,
 * \c stdout and \c stderr, this method can be used at initialization to
 * associate an UART to the standard streams.
 *
 * @sa \reflibc{group__avr__stdio.html#gab599ddf60819df4cc993c724a83cb1a4,fdevopen()} in avr-libc documentation.
 */
FILE *uart_fopen(uart_t *u);

/// \e Get function to be used with \c fdevopen()
int uart_dev_recv(FILE *);

/// \e Put function to be used with \c fdevopen()
int uart_dev_send(char c, FILE *);


#endif
//@}
