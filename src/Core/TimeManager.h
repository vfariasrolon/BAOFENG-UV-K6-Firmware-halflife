#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t last_run;
    uint32_t interval;
    void (*task_func)(void);
} Task;

extern volatile uint32_t g_SystemTick;

void TimeManager_Init(void);
void TimeManager_AddTask(void (*task_func)(void), uint32_t interval);
void TimeManager_RunScheduler(void);

#endif // TIMEMANAGER_H
