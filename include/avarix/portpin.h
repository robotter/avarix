/**
 * @file
 * @brief Tools to work with ports and their pins
 */
//@{
#ifndef AVARIX_PORTPIN_H__
#define AVARIX_PORTPIN_H__

#include <avr/io.h>
#include <stdint.h>


/// Pin of a port
typedef struct {
  PORT_t *port;
  uint8_t pin;
} portpin_t;


#ifndef DOXYGEN

/// Indirection to Return a port from its letter
#define PORTX_(x)  (PORT##x)

#endif

/// Return a portpin_t for PORTx, pin n.
#define PORTPIN(x,n)  ((portpin_t){ &PORTX_(x), n })

/// Unset \ref portpin_t value (port set to 0)
#define PORTPIN_NONE  ((portpin_t){ 0, 0 })


/** @name Update pin bit of port registers */
//@{

/// Generic macro to set a port register to the pin bitmask
#define PORTPIN_REGSET(pp,r)  ((pp)->port->r = (1 << (pp)->pin))

/// Set port pin data direction
#define PORTPIN_DIRSET(pp)  PORTPIN_REGSET(pp, DIRSET)
/// Clear port pin data direction
#define PORTPIN_DIRCLR(pp)  PORTPIN_REGSET(pp, DIRCLR)
/// Toggle port pin data direction
#define PORTPIN_DIRTGL(pp)  PORTPIN_REGSET(pp, DIRTGL)
/// Set port pin output
#define PORTPIN_OUTSET(pp)  PORTPIN_REGSET(pp, OUTSET)
/// Clear port pin output
#define PORTPIN_OUTCLR(pp)  PORTPIN_REGSET(pp, OUTCLR)
/// Toggle port ping output
#define PORTPIN_OUTTGL(pp)  PORTPIN_REGSET(pp, OUTTGL)
/// Set port pin intput
#define PORTPIN_INSET(pp)  PORTPIN_REGSET(pp, INSET)
/// Clear port pin intput
#define PORTPIN_INCLR(pp)  PORTPIN_REGSET(pp, INCLR)
/// Toggle port ping intput
#define PORTPIN_INTGL(pp)  PORTPIN_REGSET(pp, INTGL)

//@}


/// Get a pointer to the \e PINnCTRL register
#define PORTPIN_CTRL(pp)  ((&(pp)->port->PIN0CTRL)[(pp)->pin])

/// Event Channel multiplexer input selection for the port pin
#define PORTPIN_EVSYS_CHMUX(pp)  (EVSYS_CHMUX_PORTA_PIN0_gc + ((pp)->port-&PORTA) * 8 + (pp)->pin)


#endif
//@}
