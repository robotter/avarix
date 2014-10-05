/** @defgroup idle Idle tasks
 * @brief Idle tasks
 *
 * Schedule tasks to be executed when idle, for instance when waiting for a
 * given state. Tasks are distributed in execution slots based on their
 * execution cost.
 *
 * To use this module, first configure tasks in idle_config.py.
 * For instance:
 * \code{.py}
 * # Set minimum period (in microseconds)
 * set_min_period(100)  # 0.1ms
 * # Add tasks
 * add_task('always', None)    # always executed when idle
 * add_task('periodic', 100000, 5)  # executed every 10ms
 * \endcode
 *
 * Define callback methods and set them (usually in initialization code):
 * \code
 * void send_telemetry(void) { ... }
 * void init(void)
 * {
 *   ...
 *   idle_set_callback(telemetry, send_telemetry);
 * }
 * \endcode
 *
 * Finally, call idle() to execute the tasks, typically when waiting.
 */
//@{
/**
 * @file
 * @brief Idle module definitions
 */
#ifndef IDLE_H__
#define IDLE_H__


/// Callback type for idle tasks
typedef void (*idle_callback_t)(void);

#include "idle/idle_tasks.h"


/// Function to call when idle
void idle(void);

/** @brief Set the callback of a task
 *
 * If the callback is null, the task is disabled and will not be executed.
 */
#define idle_set_callback(task,callback) \
    idle_set_callback_(IDLE_TASK_##task, callback)

#ifndef DOXYGEN
void idle_set_callback_(uint8_t index, idle_callback_t cb);
#endif


#endif
//@}
