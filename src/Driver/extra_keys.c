#include "extra_keys.h"
#include "KD32f328_gpio.h"
#include "KD32f328_rcc.h"

void ExtraKeys_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Habilitar relojes para GPIOA, GPIOB y GPIOF */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOF, ENABLE);

    /* Desactivar reloj de USART1 por si el bootloader lo dejó secuestrando PA10 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);

    /* PTT: PA10 Entrada con Pull-Up */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* SideKey2: PB14 Entrada con Pull-Up */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* SideKey1: PF6 Entrada con Pull-Up */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
    GPIO_Init(GPIOF, &GPIO_InitStructure);
}

void ExtraKeys_ScanTask(void)
{
    static KeyID_Enum preExtKey = KEYID_NONE;
    static KeyID_Enum stableExtKey = KEYID_NONE;
    KeyID_Enum extKey = KEYID_NONE;

    /* Los botones son activos en bajo (Pull-Up y conectan a GND) */
    if ((GPIOA->IDR & GPIO_Pin_10) == 0) {
        extKey = KEYID_PTT;
    } else if ((GPIOF->IDR & GPIO_Pin_6) == 0) {
        extKey = KEYID_SIDEKEY1;
    } else if ((GPIOB->IDR & GPIO_Pin_14) == 0) {
        extKey = KEYID_SIDEKEY2;
    }

    if (preExtKey == extKey) {
        if (stableExtKey != extKey) {
            if (extKey != KEYID_NONE) {
                g_keyScan.keyEvent = extKey;
                g_keyScan.keyPara = KEYSTATE_CLICKED;
            }
            stableExtKey = extKey;
        }
    } else {
        preExtKey = extKey;
    }
}
