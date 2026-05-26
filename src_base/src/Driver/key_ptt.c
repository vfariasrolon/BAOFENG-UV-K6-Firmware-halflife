#include "includes.h"
#include "AppHalfLife.h"

Boolean PTT_GetKeyVal(void)
{
    if(GPIOA->IDR & GPIO_Pin_10)
    {
        return 1;
    }
    else
    {
       return 0;
    }
}


void PTT_ScanTask(void)
{
    static U8 pttCnt;
    
    // If in custom Half-Life menus, PTT press instantly exits back to main frequency VFO
    if(PTT_GetKeyVal() == 0)
    {
        if(HL_GetMode() == MODE_HL_MENU ||
           HL_GetMode() == MODE_DTMF_ANI)
        {
            HL_SetMode(MODE_MAIN);
            
            DisplayHomePage();
            RxReset();
        }
    }
    
    // Half-Life OTAP Modes intercept PTT key press to trigger provisioning
    if(HL_GetMode() == MODE_MASTER_PAIR || HL_GetMode() == MODE_SLAVE_LISTEN)
    {
        if(PTT_GetKeyVal() == 0)
        {
            pttCnt++;
            if(pttCnt >= 3)
            {
                pttCnt = 0;
                if(HL_GetMode() == MODE_MASTER_PAIR)
                {
                    MasterPairTrigger();
                }
            }
        }
        else
        {
            pttCnt = 0;
        }
        return;
    }
    
    if(g_rfState == RF_TX && g_rfTxState != TX_STOP && alarmDat.alarmStates != ON)
    {
        return;
    }
    else
    {
        if(PTT_GetKeyVal() == 0)
        {
            pttCnt++;
            if(pttCnt < 3)
            {
                return;
            }
        }
        else
        {
            pttCnt = 0;
            return;
        }

        LCD_BackLightSetOn();
        ResetTimeKeyLockAndPowerSave();

        if(alarmDat.alarmStates)
        {
            AlarmFuncSwitch(OFF);
        }

        if(Audio_CheckBusy())
        {
            Audio_PlayStop();
            return;
        }

        Radio_EnterTxMode();
    }
}

