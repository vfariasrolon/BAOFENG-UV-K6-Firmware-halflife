#include "includes.h"
#include "led.h"
#include "KD32f328_gpio.h"
#include "KD32f328_rcc.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. Habilitar reloj del puerto GPIOA en el bus AHB */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* 2. Configurar Pin 3 como Salida Push-Pull */
    GPIO_InitStructure.GPIO_Pin = LED_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_RX_PORT, &GPIO_InitStructure);

    /* 3. Estado inicial: Apagado */
    LED_RX_PORT->BRR = LED_RX_PIN;
}

void LED_Toggle(void)
{
    static U8 state = 0;
    
    if (state)
    {
        /* Apagar LED (Reset) */
        LED_RX_PORT->BRR = LED_RX_PIN;
        state = 0;
    }
    else
    {
        /* Encender LED (Set) */
        LED_RX_PORT->BSRR = LED_RX_PIN;
        state = 1;
    }
}
