#include "../Common/includes.h"
#include "TimeManager.h"
#include "../Driver/inc/KD32f328_tim.h"
#include "../Driver/inc/KD32f328_rcc.h"
#include "../Driver/inc/kd32f328xb.h"
#include "../Driver/inc/KD32f328_misc.h"

volatile uint32_t g_SystemTick = 0;

#define MAX_TASKS 10
static Task g_Tasks[MAX_TASKS];
static uint8_t g_TaskCount = 0;

void TimeManager_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // Activar reloj para TIM3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // Reloj a 96MHz. 
    // Prescaler 96-1 -> 1 MHz
    // Period 1000-1 -> 1 kHz (1 ms)
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 96 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // Activar interrupción
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    // Configurar NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Iniciar el temporizador
    TIM_Cmd(TIM3, ENABLE);
}

void TIM3_IRQHandler(void) {
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        g_SystemTick++;
    }
}

void TimeManager_AddTask(void (*task_func)(void), uint32_t interval) {
    if (g_TaskCount < MAX_TASKS) {
        g_Tasks[g_TaskCount].task_func = task_func;
        g_Tasks[g_TaskCount].interval = interval;
        g_Tasks[g_TaskCount].last_run = g_SystemTick;
        g_TaskCount++;
    }
}

void TimeManager_RunScheduler(void) {
    for (uint8_t i = 0; i < g_TaskCount; i++) {
        if ((g_SystemTick - g_Tasks[i].last_run) >= g_Tasks[i].interval) {
            g_Tasks[i].last_run = g_SystemTick;
            if (g_Tasks[i].task_func) {
                g_Tasks[i].task_func();
            }
        }
    }
}
