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
/// Return the length of an array (fails on pointer)
#define ARRAY_LEN(a)  ({ \
    _Static_assert(!__builtin_types_compatible_p(typeof(a), typeof(&a[0])), "expected array (not pointer)"); \
    sizeof(a)/sizeof(a[0]); \
  })

#endif
//@}
