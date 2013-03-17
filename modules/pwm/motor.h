/** @defgroup pwm PWM
 * @brief PWM module
 */
//@{
/**
 * @file
 * @brief PWM for motors
 */
#ifndef PWM_MOTOR_H__
#define PWM_MOTOR_H__

#include <avr/io.h>
#include <avarix/portpin.h>

/// Maximum value of PWM duty cycle
#define PWM_MOTOR_MAX  32767



/** @brief Motor PWM data
 * @note Fields are private and should not be accessed directly.
 */
typedef struct {
  TC0_t *tc;  ///< timer used to generate the PWM signal
  uint8_t channel;  ///< timer channel used for PWM output, from 0 to 3
  portpin_t signpp;  ///< port pin of sign output (optional)
  uint16_t vmin;  ///< duty cycle range, lower bound, in ticks
  uint16_t vmax;  ///< duty cycle range, upper bound, in ticks
} pwm_motor_t;


/** @brief Initialize motor PWM
 *
 * @param pwm  motor PWM to initialize
 * @param tc  timer to use (can also be a pointer to a \e TC1_t)
 * @param channel  timer channel to use, letter from A to D
 * @param pwmpp  port pin of PWM output
 * @param signpp  port pin of sign output, \ref PORTPIN_NONE if not used
 *
 * \e pwmpp depends on the XMEGA device and must match \e tc and \e channel.
 * For timer \e TCxn, channel \e y it must designates pin \e OCny on port \e x.
 */
void pwm_motor_init(pwm_motor_t *pwm, TC0_t *tc, char channel, portpin_t pwmpp, portpin_t signpp);

/** @brief Set motor PWM output frequency, in hertz
 *
 * This method should be called before using the PWM.
 *
 * PWM frequency is configured per timer. PWMs configured on the same timer
 * share the same frequency. It is advised to set frequency of each PWM motor,
 * even if they use the same timer.
 *
 * Timer prescaler is not used and frequency range depends only on
 * \ref CLOCK_PER_FREQ. Values can go from <em>CLOCK_PER_FREQ/65536</em> to
 * <em>CLOCK_PER_FREQ/2</em>.
 * Moreover, type range restrict frequency maximum value to 65535Hz.
 *
 * @note The duty cycle period range is clamped if needed.
 */
void pwm_motor_set_frequency(pwm_motor_t *pwm, uint16_t freq);

/** @brief Set duty cycle period range, in microseconds
 *
 * Value provided to pwm_motor_set() will be scaled to that range.
 *
 * The range is clamped if needed. Thus, providing (0, 65535) will reset the
 * range to largest value range.
 *
 * @note This method should be called after pwm_motor_set_frequency().
 */
void pwm_motor_set_range(pwm_motor_t *pwm, uint16_t tmin, uint16_t tmax);

/** @brief Set motor duty cycle
 *
 * Resulting period range can be configured using pwm_motor_set_range().
 * Sign is handled independently.
 *
 * If sign output is configured, value should be positive.
 */
void pwm_motor_set(pwm_motor_t *pwm, int16_t v);


#endif
//@}
