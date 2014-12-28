/**
 * @file
 * @brief Internal definitions for internal needs
 */
//@{
#ifndef AVARIX_INTERNAL_H__
#define AVARIX_INTERNAL_H__


/** @name Preprocessor concatenation indirections */
//@{

#define AVARIX_CONCAT2(a,b)  a ## b
#define AVARIX_EVALCONCAT2(a,b)  AVARIX_CONCAT2(a,b)

#define AVARIX_CONCAT3(a,b,c)  a ## b ## c
#define AVARIX_EVALCONCAT3(a,b,c)  AVARIX_CONCAT3(a,b,c)

//@}

/** @brief Instruct to not 0-initialize a global variable
 *
 * Some global variables such as large buffers don't need to initialied.
 * Not initializing them reduces program space.
 *
 * @note The \c .noinit section doesn't work as expected.
 * Data is correctly put in it but it still initialized.
 */
#define AVARIX_DATA_NOINIT   __attribute__ ((section (".data")))

#endif
//@}
