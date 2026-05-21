#include "watchdog.h"
#include "KD32f328_iwdg.h"
#include "KD32f328_rcc.h"
#include "KD32f328_gpio.h"

void WDT_Init(void)
{
    /* Detección de Modo Recovery: 
     * Si el usuario mantiene pulsado PTT (PA10 = 0) al encender la radio, 
     * abortamos el inicio del Watchdog para evitar reboots durante un flasheo.
     */
    if ((GPIOA->IDR & GPIO_Pin_10) == 0) {
        return;
    }

    /* Activar el reloj LSI interno (típicamente 40kHz) */
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    /* Desbloquear acceso a los registros del perro guardián */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* Configurar Prescaler: 64 (40kHz / 64 = 625 Hz) */
    IWDG_SetPrescaler(IWDG_Prescaler_64);

    /* Configurar Reload: 2500 (2500 / 625 Hz = 4.0 segundos exactos) */
    IWDG_SetReload(2500);

    /* Recargar contador con el nuevo valor antes de arrancar */
    IWDG_ReloadCounter();

    /* Arrancar el perro guardián (Protección activada) */
    IWDG_Enable();
}

void WDT_Refresh(void)
{
    /* Alimentar al perro para reiniciar la cuenta de 4 segundos */
    IWDG_ReloadCounter();
}
