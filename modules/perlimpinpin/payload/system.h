/** @addtogroup perlimpinpin */
//@{
/** @file
 * @brief Perlimpinpin system payload support
 */
#ifndef PERLIMPINPIN_PAYLOAD_SYSTEM_H__
#define PERLIMPINPIN_PAYLOAD_SYSTEM_H__

#include "../perlimpinpin.h"


/// System payload handler
int8_t ppp_payload_handler_system(ppp_intf_t *intf);

/// Send a system ACK or NACK
void ppp_send_system_ack(ppp_intf_t *intf, bool ack);

/// Broadcast a system RESET
void ppp_send_system_reset(ppp_intf_t *intf);


#endif
//@}
