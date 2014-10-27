#include <stdio.h>

#define IDLE_PERIODIC_TASKS_END  $$avarix:self.periodic_tasks_end()$$
#define IDLE_ALWAYS_TASKS_COUNT  $$avarix:self.idle_always_tasks_size()$$

#if IDLE_ALWAYS_TASKS_COUNT > 0
static idle_callback_t idle_always_callbacks[IDLE_ALWAYS_TASKS_COUNT];
#endif

#if IDLE_PERIODIC_TASKS_END > 0
static idle_periodic_task_t idle_periodic_tasks[] = {
#pragma avarix_tpl self.idle_periodic_tasks()
};
#endif

