#include "light_system.h"
#include "KD32f328_gpio.h"
#include "KD32f328_rcc.h"

static U8 g_backlightState = 1;
static U8 g_ledTopState = 1;

void LightSystem_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. Habilitar reloj para GPIOB (Linterna) y GPIOF (Retroiluminación) */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOF, ENABLE);

    /* 2. Configuración general de pines como Salida Push-Pull */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* 3. Inicializar Linterna (PB7) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 4. Inicializar Retroiluminación LCD (PF7) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOF, &GPIO_InitStructure);

    /* 5. Estado inicial: Encendido por defecto para validación */
    GPIOB->BSRR = GPIO_Pin_7; // Linterna ON
    GPIOF->BSRR = GPIO_Pin_7; // LCD Backlight ON
    g_backlightState = 1;
    g_ledTopState = 1;
}

void Light_BacklightToggle(void)
{
    if (g_backlightState) {
        GPIOF->BRR = GPIO_Pin_7; // Apagar
        g_backlightState = 0;
    } else {
        GPIOF->BSRR = GPIO_Pin_7; // Encender
        g_backlightState = 1;
    }
}

void Light_LedTopToggle(void)
{
    if (g_ledTopState) {
        GPIOB->BRR = GPIO_Pin_7; // Apagar
        g_ledTopState = 0;
    } else {
        GPIOB->BSRR = GPIO_Pin_7; // Encender
        g_ledTopState = 1;
    }
}

void LightSystem_Set(uint8_t on)
{
    if (on) {
        GPIOF->BSRR = GPIO_Pin_7; // LCD Backlight ON
        g_backlightState = 1;
    } else {
        GPIOF->BRR = GPIO_Pin_7; // LCD Backlight OFF
        g_backlightState = 0;
    }
}
