/** @addtogroup perlimpinpin */
//@{
/** @file
 * @brief Perlimpinpin configuration
 */
/** @name Configuration
 */
//@{


/** @brief Size of ppp_intf_rstate_t::payload
 *
 * If unset or null the feature is disabled, saving some place on the
 * ppp_intf_t structure.
 */
#define PPP_PAYLOAD_BUF_SIZE  32

/** @brief Disabled interrupt level when sending PPP frames
 *
 * If PPP frames are sent from the main and and an interrupt routine, messages
 * will be mixed up.
 * If this value is set, PPP messages can be safely sent from any interrupt of
 * configured (or lower) level.
 *
 * @sa PPP_SEND_INTLVL_DISABLE()
 */
#define PPP_SEND_INTLVL


/// Perlimpinpin node name
#define PPP_NODE_NAME  "node"


/** @brief Support payload named \c X
 *
 * Valid payload names are defined in \ref payloads.h.
 *
 * Macro value must be empty.
 *
 * @note The \c SYSTEM payload is always defined.
 */
#define PPP_SUPPORT_X


//@}
//@}
