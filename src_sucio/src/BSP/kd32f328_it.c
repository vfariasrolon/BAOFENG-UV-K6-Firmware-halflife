
#include "kd32f328_it.h"
#include "includes.h"

void NMI_Handler(void)
{
}

// NASA Standard HardFault Telemetry Log for Cortex-M0
typedef struct {
    uint32_t count;   // Reset/Fault count
    uint32_t pc;      // Program Counter where fault occurred
    uint32_t lr;      // Link Register
} HardFaultLog_t;

__attribute__((section(".noinit"))) HardFaultLog_t g_faultLog;

void HardFault_Handler(void)
{
    // Extract stacked PC and LR from MSP/PSP in assembly
    __asm volatile (
        "mov r0, lr \n"
        "movs r1, #4 \n"
        "tst r0, r1 \n"
        "beq use_msp \n"
        "mrs r0, psp \n"
        "b get_frame \n"
        "use_msp: \n"
        "mrs r0, msp \n"
        "get_frame: \n"
        "ldr r1, [r0, #24] \n" // Stacked PC
        "ldr r2, [r0, #20] \n" // Stacked LR
        "ldr r3, =g_faultLog \n"
        "str r1, [r3, #4] \n"  // Save to g_faultLog.pc
        "str r2, [r3, #8] \n"  // Save to g_faultLog.lr
    );

    g_faultLog.count++;

    NVIC_SystemReset();
}


void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    UserVectors[0].intHandle();
}

void USART1_IRQHandler(void)
{
    UserVectors[1].intHandle();
}

void USART2_IRQHandler(void)
{
    UserVectors[2].intHandle();
}

void DMA1_Channel4_5_IRQHandler(void)
{
    UserVectors[3].intHandle(); 
}

