#include "includes.h"
#include "stdio.h"

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

int main(void)
{   
    Board_Init();    
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
        LCD_DisplayText(15, 12, (U8 *)"OTAP ENLACE", FONTSIZE_16x16, LCD_DIS_NORMAL);
        LCD_DisplayText(35, 12, (U8 *)"PULSE [A/B] CONFIRMAR", FONTSIZE_12x12, LCD_DIS_NORMAL);
        
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
    
    while(1)
    {
        //10ms运行一次
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
        AlarmTask();
    }
}

