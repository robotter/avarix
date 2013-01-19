/** @addtogroup perlimpinpin */
//@{
/** @file
 * @brief Perlimpinpin log payload support
 */
#ifndef PERLIMPINPIN_PAYLOAD_LOG_H__
#define PERLIMPINPIN_PAYLOAD_LOG_H__

#include "../perlimpinpin.h"


/// Log severities
enum {
  PPP_LOG_DEBUG = 0,
  PPP_LOG_NOTICE = 1,
  PPP_LOG_INFO = 2,
  PPP_LOG_WARNING = 3,
  PPP_LOG_ERROR = 4,
};

/// Value to XOR to severity to request an ACK reply
#define PPP_LOG_ACK  0x80

#ifndef PPP_LOG_FORMAT_MAX_SIZE
/** @brief Maximum size of a printf-formatted log messages
 *
 * Another value can be defined in configuration.
 */
#define PPP_LOG_FORMAT_MAX_SIZE  255
#endif


/// Send a log message
void ppp_send_log(ppp_intf_t *intf, uint8_t dst, uint8_t sev, const char *msg, uint16_t len);

/// Send a log message using a printf format
void ppp_send_logf(ppp_intf_t *intf, uint8_t dst, uint8_t sev, const char *fmt, ...)
  __attribute__((format(printf, 4, 5)));


/** @brief Broadcast a log message
 *
 * \e sev is a severity name without the \c PPP_LOG_ prefix (e.g. \c ERROR).
 * \e msg must be a literal string. No ACK is requested.
 *
 * @note PPP_LOG is more efficient than PPP_LOGF when there is no parameter to
 * format.
 */
#define PPP_LOG(intf,sev,msg) \
    ppp_send_log(intf, 0xFF, PPP_LOG_##sev, msg, sizeof(msg)-1)

/** @brief Broadcast a printf-formatted log message
 *
 * \e sev is a severity name without the \c PPP_LOG_ prefix (e.g. \c ERROR).
 */
#define PPP_LOGF(intf,sev,fmt,...) \
    ppp_send_logf(intf, 0xFF, PPP_LOG_##sev, fmt, ##__VA_ARGS__)


#endif
//@}
