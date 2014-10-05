#include <stdint.h>
#include "idle.h"


/// Idle task data
typedef struct {
  /// Function called when task is executed
  idle_callback_t callback;
  /// Number of idle() call between executions
  uint8_t period;
  /// Offset at which the task is executed
  uint8_t offset;

} idle_task_t;


#include "idle/idle_tasks.inc.c"


void idle(void)
{
  static uint16_t counter;
  for(uint8_t i=0; i<sizeof(idle_tasks)/sizeof(*idle_tasks); i++) {
    if(idle_tasks[i].callback) {
      if(counter % idle_tasks[i].period == idle_tasks[i].offset) {
        idle_tasks[i].callback();
      }
    }
  }
  counter++;
}


void idle_set_callback_(uint8_t index, idle_callback_t cb)
{
  idle_tasks[index].callback = cb;
}

