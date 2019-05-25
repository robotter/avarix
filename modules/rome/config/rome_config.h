/** @addtogroup rome */
//@{
/** @file
 * @brief ROME configuration
 */
/** @name Configuration
 */
//@{

/** @brief If set, enable use of interface based on XBee API
 *
 * @note The \e xbee module must be enabled by the user.
 */
#undef ROME_ENABLE_XBEE_API

/** @brief Disabled interrupt level when sending ROME frames
 *
 * If ROME frames are sent from the main and and an interrupt routine, frames
 * will be mixed up.
 * If this value is set, ROME frames can be safely sent from any interrupt of
 * configured (or lower) level.
 *
 * @note This is not used for data sent using XBee API.
 *
 * @sa ROME_SEND_INTLVL_DISABLE()
 */
#define ROME_SEND_INTLVL  INTLVL_LO

/// Minimum ACK value to use for sent orders
#define ROME_ACK_MIN
/// Maximum ACK value to use for sent orders
#define ROME_ACK_MAX

/// ACK waiting time before resending an order, in microseconds
#define ROME_ACK_TIMEOUT_US  500000

/// If defined, disable sending of messages X
#define ROME_DISABLE_X


//@}
//@}
