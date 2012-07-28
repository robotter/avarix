/** @addtogroup perlimpinpin */
//@{
/**
 * @file
 * @brief Perlimpinpin payloads types
 */
#ifndef PERLIMPINPIN_PAYLOADS_H__
#define PERLIMPINPIN_PAYLOADS_H__


/** @brief Apply an expression for all known payload types
 *
 * The expression is a function macro with two parameters: the payload type ID
 * and the payload name (uppercase).
 *
 * @note IDs must be written using the \c 0x%02X format.
 */
#define PPP_TYPE_FOREACH(e) \
    e(0x00, SYSTEM) \
    e(0x01, LOG) \
    e(0x10, ROOM) \
    //


/** @cond */
#define EXPR(v,n)  PPP_TYPE_ ## n = v,
/** @endcond */
/// Payload types
enum {
  PPP_TYPE_FOREACH(EXPR)
};
#undef EXPR


#endif
//@}
