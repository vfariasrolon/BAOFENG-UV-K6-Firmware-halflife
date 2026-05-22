#include "includes.h"
#include "KD32f328_iwdg.h"
#include "AppHalfLife.h"
#include "AppEventManager.h"

void App_10msTask(void)
{
    g_10msFlag = FALSE;

    // Feed independent hardware watchdog
    IWDG_ReloadCounter();

    RF_Task();
    KEY_ScanTask();
    ExtraKeys_ScanTask();
    PTT_ScanTask();
    
    
    // Half-Life Background Telemetry scan
    BackgroundTelemetryTask();
}

extern void App_50msTask(void)
{
    g_50msFlag = FALSE;

    if (g_uiState != UI_STATE_TEST_BENCH) {
        ScanTask();
        DualStandbyTask();
    }
    
    LCD_CheckBackLight();
}

extern volatile U8 g_vrfr_tx_blink_counter;

extern void App_100msTask(void)
{
    g_100msFlag = FALSE;
    
    /* Procesar parpadeo no bloqueante de transmisión VRFR */
    if (g_vrfr_tx_blink_counter > 0) {
        LED_Toggle();
        g_vrfr_tx_blink_counter--;
    }
    
    CheckExitMenu();
    CheckPowerOff();
    CheckAutoKeyLockTask();
    LightFlashTask();
    VoxCheckTask();
}

extern void App_500msTask(void)
{
    g_500msFlag = FALSE;
    BatteryCheckTask();
    
    if (g_uiState != UI_STATE_TEST_BENCH) {
        CalculateSqlLevel();
    }
    
    CheckSjTimeout();
}

extern void AppRunTask(void)
{
    /* Despachador Minimalista de Eventos (FSM) */
    if (g_keyScan.keyEvent != KEYID_NONE) {
        App_EventManager(g_keyScan.keyEvent);
        
        /* Limpiar buffer de entrada para no repetir el procesamiento */
        g_keyScan.keyEvent = KEYID_NONE;
    }
}


