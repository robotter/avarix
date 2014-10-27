#include <stdint.h>
#include <timer/uptime.h>
#include "idle.h"


/// Idle peridoic task data
typedef struct {
  /// Function called when task is executed
  idle_callback_t callback;
  /// Executiod period, in microseconds
  uint32_t period;
  /// Uptime of the next execution
  uint32_t next;

} idle_periodic_task_t;


#include "idle/idle_tasks.inc.c"


void idle(void)
{
#if IDLE_ALWAYS_TASKS_COUNT > 0
  for(uint8_t i=0; i<IDLE_ALWAYS_TASKS_COUNT; i++) {
    if(idle_always_callbacks[i]) {
      idle_always_callbacks[i]();
    }
  }
#endif

#if IDLE_PERIODIC_TASKS_END > 0
  uint32_t now = uptime_us();
  for(uint8_t i=0; i<IDLE_PERIODIC_TASKS_END; i++) {
    idle_periodic_task_t *task = &idle_periodic_tasks[i];
    if(task->callback) {
      if(now >= task->next) {
        task->callback();
        task->next += task->period;
      }
    }
  }
#endif
}


void idle_set_callback_(uint8_t index, idle_callback_t cb)
{
#if IDLE_ALWAYS_TASKS_COUNT > 0 && IDLE_PERIODIC_TASKS_END > 0
  if(index >= IDLE_PERIODIC_TASKS_END)
#endif
#if IDLE_ALWAYS_TASKS_COUNT > 0
  {
    idle_always_callbacks[index-IDLE_PERIODIC_TASKS_END] = cb;
  }
#endif
#if IDLE_ALWAYS_TASKS_COUNT > 0 && IDLE_PERIODIC_TASKS_END > 0
  else
#endif
#if IDLE_PERIODIC_TASKS_END > 0
  {
    idle_periodic_tasks[index].callback = cb;
    idle_periodic_tasks[index].next = uptime_us();
  }
#endif
}

