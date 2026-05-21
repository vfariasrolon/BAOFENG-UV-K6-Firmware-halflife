#include "includes.h"
#include "stdio.h"
#include "AppEventManager.h"
#include "../Driver/minifont.h"
#include "../Driver/watchdog.h"

void BeepPowerOn(void)
{
    if(g_radioInform.OpFlag1.Bit.b2 == 1)
    {
        BeepOut(BEEP_FMSW2);
    }
    else if(g_radioInform.OpFlag1.Bit.b2 == 2)
    {
        Audio_PlayVoiceLock(vo_Welcome);
    }
    else
    {
        DelayMs(300);
    }
}

#include "KD32f328_gpio.h"

U8 Debug_ReadPTT(void)
{
    if ((GPIOA->IDR & GPIO_Pin_10) == 0) {
        return 0; // PTT Presionado
    } else {
        return 1; // PTT Suelto
    }
}

int main(void)
{   
    Board_Init();    
    LED_Init(); // Inicializamos el LED para el Blink Test
    Keyboard_Init(); // Inicializamos el Teclado para recibir entradas
    ExtraKeys_Init(); // Inicializamos PTT y botones laterales
    LightSystem_Init(); // Inicializar Linterna y Retroiluminación (ON por defecto)
    SPI2_Init();      // Inicializar el bus SPI
    SC5260_Init();    // Inicializar la pantalla LCD

    LCD_DrawLogo();
    LCD_UpdateFullScreen();
    DelayMs(2000);
    SC5260_ClearArea(0, 0, 128, 64, 0);

    LCD_RunDiagnosticTest(); // Pruebas visuales geométricas
    LCD_ShowAlphabetTest();  // Prueba de tipografía en 3 escalas
    RadioConfig_Init();
    g_radioInform.language = LANG_EN; // Force English language globally to remove all Chinese voice and menus
    UI_DisplayPowerOn();
    Rfic_Init();
    ChannelCheckActiveAll();
    BeepPowerOn();
    BatteryInitLevel();

    App_CheckPowerOnPassword();

    //CheckHiddenParaSet(); 

    if(GetKeyCode() == KEYID_8)
    {
        DisplaySoftVersion();
    }
    
    // Half-Life OTAP Slave activation (Hold Side Key 2 at boot and press A/B)
    if(GetKeyCode() == KEYID_SIDEKEY2)
    {
        BeepOut(BEEP_FASTSW);
        SC5260_ClearArea(0, 0, 128, 64, 0);
        UI_DrawText(15, 12, "OTAP ENLACE", SCALE_NORMAL);
        UI_DrawText(15, 30, "PULSE [A/B] CONFIRMAR", SCALE_NORMAL);
        
        U16 timeout = 0;
        while(timeout < 200) // 2 seconds window
        {
            DelayMs(10);
            if(GetKeyCode() == KEYID_AB)
            {
                HL_SetMode(MODE_SLAVE_LISTEN);
                BeepOut(BEEP_FMSW2);
                break;
            }
            timeout++;
        }
    }

    RadioVfoInfo_Init();

    ResetTimeKeyLockAndPowerSave();
    ResetInputBuf();

    //初始化写频模式
    ProgromInit();
    LCD_BackLightSetOn();

    g_rfState = RF_RX;
    g_rfTxState = TX_READY;
    g_rfRxState = RX_READY;
    g_scanInfo.state = SCAN_IDLE;
    
    if (HL_GetMode() != MODE_SLAVE_LISTEN)
    {
        HL_SetMode(MODE_MAIN);
    }
    g_keyScan.keyEvent = KEYID_NONE;
    
    // Hardware Watchdog activation: delay until all slow startup tasks complete
    WDT_Init();
    
    g_uiState = UI_STATE_DEBUG_MAPPING;
    Light_LedTopToggle();
    SC5260_ClearArea(0, 0, 128, 64, 0);
    UI_DrawText(4, 20, "DEBUG MODE", SCALE_NORMAL);
    LCD_UpdateFullScreen();
    
    while(1)
    {
        // Forzamos el polling aquí porque g_10msFlag no está disparando (SysTick apagado)
        KEY_ScanTask();
        ExtraKeys_ScanTask();

        // 10ms运行一次
        if(g_10msFlag)
        {
            App_10msTask();
        }
        
        if(g_50msFlag)
        {
            App_50msTask();
        }
        
        //100ms运行一次
        if(g_100msFlag)
        {
            App_100msTask();
        }

        //500ms运行一次
        if(g_500msFlag)
        {
            App_500msTask();
        }
        
        AppRunTask();
        
        WDT_Refresh();
    }
}

