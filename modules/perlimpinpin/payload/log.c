/**
 * @cond internal
 * @file
 */
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

#define PPP_LOG_PLTYPE  0x01


void ppp_send_log(ppp_intf_t *intf, uint8_t dst, uint8_t sev, const char *msg, uint16_t len)
{
  const ppp_header_t header = {
    .plsize = 1+len,
    .src = intf->addr,
    .dst = dst,
    .pltype = PPP_LOG_PLTYPE,
  };
  ppp_send_frame_header(intf, &header);
  ppp_send_frame_data(intf, &sev, 1);
  ppp_send_frame_data(intf, msg, len);
  ppp_send_frame_crc(intf);
}


void ppp_send_logf(ppp_intf_t *intf, uint8_t dst, uint8_t sev, const char *fmt, ...)
{
  // +1 for sev, +1 for trailing NUL
  char buf[PPP_LOG_FORMAT_MAX_SIZE+2] = { sev };

  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
  va_end(ap);
  if( len < 0 ) {
    return; // error, silently abort
  }

  const ppp_header_t header = {
    .plsize = 1+len,
    .src = intf->addr,
    .dst = dst,
    .pltype = PPP_LOG_PLTYPE,
  };
  ppp_send_frame(intf, &header, buf);
}

//@endcond
