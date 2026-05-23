#include "includes.h"
#include "stdio.h"
#include "AppEventManager.h"
#include "../Driver/minifont.h"
#include "../Driver/watchdog.h"
#include "../Driver/BK4829_Minimal.h"
#include "../Core/TimeManager.h"
#include "../Protocol/vrfr_proto.h"
#include "Auth.h"
#include "../Core/AudioEngine.h"
#include "kd32f328xb.h"
#include "KD32f328_gpio.h"

void BeepPowerOn(void)
{
    AudioEngine_PlayNextelChirp();
}

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
    WDT_Init(); // Arrancar WDT temprano

    extern void Usart_Init(void);
    SystemCoreClockUpdate();
    Usart_Init();

    Board_Init();    
    LED_Init(); // Inicializamos el LED para el Blink Test
    Keyboard_Init(); // Inicializamos el Teclado para recibir entradas
    ExtraKeys_Init(); // Inicializamos PTT y botones laterales
    LightSystem_Init(); // Inicializar Linterna y Retroiluminación
    SPI2_Init();      // Inicializar el bus SPI
    SC5260_Init();    // Inicializar la pantalla LCD

    TimeManager_Init(); // Arrancar TIM3 y Scheduler
    
    // ---------------------------------------------------------
    // RELOCACIÓN DE VECTOR TABLE (CRÍTICO PARA CORTEX-M0)
    uint32_t *vectors_flash = (uint32_t *)0x08002000;
    uint32_t *vectors_sram  = (uint32_t *)0x20000000;
    for (int i = 0; i < 48; i++) {
        vectors_sram[i] = vectors_flash[i];
    }
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitar reloj SYSCFG
    SYSCFG->CFGR1 |= 0x03; // SYSCFG_MemoryRemap_SRAM
    // ---------------------------------------------------------
    
    // Purgar basura del bootloader
    SysTick->CTRL = 0; 
    SCB->ICSR = (1 << 25);
    NVIC->ICPR[0] = 0xFFFFFFFF;
    
    // Habilitar interrupciones globales
    __enable_irq();

    SystemCoreClockUpdate();
    extern void Usart_Init(void);
    Usart_Init();

    LCD_DrawLogo();
    LCD_UpdateFullScreen();
    
    // Espera no bloqueante usando SystemTick
    uint32_t logo_start = g_SystemTick;
    while((g_SystemTick - logo_start) < 2000) {
        WDT_Refresh();
    }
    SC5260_ClearArea(0, 0, 128, 64, 0);

    LCD_RunDiagnosticTest(); // Pruebas visuales geométricas
    LCD_ShowAlphabetTest();  // Prueba de tipografía en 3 escalas
    RadioConfig_Init();
    g_radioInform.language = LANG_EN; // Force English language globally
    UI_DisplayPowerOn();
    
    // Inicializar tácticas y audio
    AudioEngine_Init(); 
    
    ChannelCheckActiveAll();
    BeepPowerOn();
    BatteryInitLevel();

    App_CheckPowerOnPassword();

    if(GetKeyCode() == KEYID_8)
    {
        DisplaySoftVersion();
    }
    
    // Half-Life OTAP Slave activation
    if(GetKeyCode() == KEYID_SIDEKEY2)
    {
        BeepOut(BEEP_FASTSW);
        SC5260_ClearArea(0, 0, 128, 64, 0);
        UI_DrawText(15, 12, "OTAP ENLACE", SCALE_NORMAL);
        UI_DrawText(15, 30, "PULSE [A/B] CONFIRMAR", SCALE_NORMAL);
        uint32_t otap_start = g_SystemTick;
        while((g_SystemTick - otap_start) < 2000) // 2 seconds window
        {
            if(GetKeyCode() == KEYID_AB)
            {
                HL_SetMode(MODE_SLAVE_LISTEN);
                BeepOut(BEEP_FMSW2);
                break;
            }
            WDT_Refresh();
        }
    }

    RadioVfoInfo_Init();

    ResetTimeKeyLockAndPowerSave();
    ResetInputBuf();

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
    
    WDT_Init();
    
    // ========================================================
    // RE-INICIALIZACIÓN FORZADA (Evita sobrescritura de OEM)
    // ========================================================
    BK4829_Init(); 
    Auth_Init();
    VRFR_Init(); // Arrancar FSK y variables
    // ========================================================
    
    // Forzar UI a Test Bench
    g_uiState = UI_STATE_TEST_BENCH;
    SC5260_ClearArea(0, 0, 128, 64, 0); // Limpiar OEM UI
    VRFR_RenderDiagnostics(); // Renderizar por primera vez
    LCD_UpdateFullScreen();
    
    // Registrar Tareas
    TimeManager_AddTask(VRFR_Tick, 10); // Lógica RX/TX DTMF
    TimeManager_AddTask(AudioEngine_Task, 1); // Gestor de secuencias de audio
    
    // El envío FSK automático se ha desactivado para permitir pruebas manuales con la Matriz FSK.
    while(1)
    {
        TimeManager_RunScheduler();
        
        KEY_ScanTask();
        ExtraKeys_ScanTask();
        
        // Mapeo Rápido de Teclas Locales hacia VRFR
        if (g_keyScan.keyEvent != KEYID_NONE) {
            uint8_t code = g_keyScan.keyEvent;
            g_keyScan.keyEvent = KEYID_NONE; // Consumir evento
            
            if (code >= KEYID_1 && code <= KEYID_9) {
                VRFR_ProcessLocalKey(code - KEYID_1 + 1);
            } else if (code == KEYID_0) {
                VRFR_ProcessLocalKey(0);
            }
        }
        
        if(g_10msFlag) { App_10msTask(); }
        if(g_50msFlag) { App_50msTask(); }
        if(g_100msFlag) { App_100msTask(); }
        if(g_500msFlag) { App_500msTask(); }
        
        AppRunTask();
        WDT_Refresh();
    }
}


