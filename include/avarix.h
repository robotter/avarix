/**
 * @file
 * @brief Various common Avarix definitions
 */
#ifndef AVARIX_H__
#define AVARIX_H__


/// Get minimum value out of two values
#define MIN(x,y) ((x) < (y) ? (x) : (y))
/// Get maximum value out of two values
#define MAX(x,y) ((x) > (y) ? (x) : (y))
/// Clamp a value to range [x;y]
#define CLAMP(v,x,y) ((v) < (x) ? (x) : (v) > (y) ? (y) : (v))
/// Get number of elements in an array
#define LENGTHOF(a) ((sizeof(a)/sizeof((a)[0]))/(sizeof(a)%sizeof((a)[0]) == 0))

#endif
//@}
