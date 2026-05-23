#include "includes.h"
#include "KD32f328_iwdg.h"
#include "AppHalfLife.h"
#include "AppEventManager.h"
#include "../Protocol/vrfr_proto.h"
#include "../Driver/Sc5260.h"

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
    
    if (g_uiState != UI_STATE_TEST_BENCH) {
        BatteryCheckTask();
        CalculateSqlLevel();
    } else {
        // Forzar redibujado de nuestra UI cada 500ms por si alguna tarea OEM ensucia la pantalla
        VRFR_RenderDiagnostics();
        LCD_UpdateFullScreen();
    }
    
    CheckSjTimeout();
}

extern void AppRunTask(void)
{
    /* Despachador Minimalista de Eventos (FSM) */
    if (g_keyScan.keyEvent != KEYID_NONE) {
        uint8_t code = g_keyScan.keyEvent;
        
        // Mapeo Rápido de Teclas Locales hacia VRFR
        if (code >= KEYID_1 && code <= KEYID_9) {
            VRFR_ProcessLocalKey(code - KEYID_1 + 1);
        } else if (code == KEYID_0 || code == KEYID_PTT) {
            VRFR_ProcessLocalKey(0);
        } else {
            // Pasar al sistema operativo original solo si no lo consumimos
            App_EventManager(code);
        }
        
        /* Limpiar buffer de entrada para no repetir el procesamiento */
        g_keyScan.keyEvent = KEYID_NONE;
    }
}


