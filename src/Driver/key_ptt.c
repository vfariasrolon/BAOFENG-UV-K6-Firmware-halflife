#include "includes.h"

extern Boolean PTT_GetKeyVal(void)
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


extern void PTT_ScanTask(void)
{
    static U8 pttCnt;
    
    // If in custom Half-Life menus, PTT press instantly exits back to main frequency VFO
    if(PTT_GetKeyVal() == 0)
    {
        if(g_sysRunPara.sysRunMode == MODE_HL_MENU ||
           g_sysRunPara.sysRunMode == MODE_DASHBOARD ||
           g_sysRunPara.sysRunMode == MODE_DTMF_ANI)
        {
            g_sysRunPara.sysRunMode = MODE_MAIN;
            extern void DisplayHomePage(void);
            extern void RxReset(void);
            DisplayHomePage();
            RxReset();
        }
    }
    
    // Half-Life OTAP Modes intercept PTT key press to trigger provisioning
    if(g_sysRunPara.sysRunMode == MODE_MASTER_PAIR || g_sysRunPara.sysRunMode == MODE_SLAVE_LISTEN)
    {
        if(PTT_GetKeyVal() == 0)
        {
            pttCnt++;
            if(pttCnt >= 3)
            {
                pttCnt = 0;
                if(g_sysRunPara.sysRunMode == MODE_MASTER_PAIR)
                {
                    extern void MasterPairTrigger(void);
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

