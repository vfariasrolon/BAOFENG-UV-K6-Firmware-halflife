#include "includes.h"
#include "AppHalfLife.h"

extern void App_10msTask(void)
{
    g_10msFlag = FALSE;

    RF_Task();
    KEY_ScanTask();
    PTT_ScanTask();
    FmTaskFunc();
    
    // Half-Life Background Telemetry scan
    BackgroundTelemetryTask();
}

extern void App_50msTask(void)
{
    g_50msFlag = FALSE;

    ScanTask();
    WeatherScanTask();
    DualStandbyTask();
    StopWatchDisplayTime();
    LCD_CheckBackLight();
}

extern void App_100msTask(void)
{
    g_100msFlag = FALSE;
    SearchFreqTask();
    TaskRemoteScanQT();
    CheckExitMenu();
    CheckPowerOff();
    CheckAutoKeyLockTask();
    CheckAutoKeyLockTask(); // redundant but safe
    LightFlashTask();
    VoxCheckTask();
}

extern void App_500msTask(void)
{
    g_500msFlag = FALSE;
    BatteryCheckTask();
    CalculateSqlLevel();
    CheckSjTimeout();
}

extern void AppRunTask(void)
{
    U8 keyEvent;
    
    // Safety sanitization of active g_CurrentVfo pointers
    HL_SanitizeVfoPointers();
    
    // Half-Life Special modes loop execution
    if (g_sysRunPara.sysRunMode == MODE_SLAVE_LISTEN)
    {
        SlaveListenTask();
        Audio_PlayTask();
        return;
    }
    else if (g_sysRunPara.sysRunMode == MODE_MASTER_PAIR)
    {
        MasterPairTask();
        Audio_PlayTask();
        return;
    }
    
    switch(g_rfState)
    {
        case RF_TX:
            Radio_TxKeyTone(g_keyScan.keyEvent,g_keyScan.keyPara);
            break;
        case RF_RX:
        default:
           if(g_keyScan.keyEvent != KEYID_NONE)
           {
               keyEvent = Key_GetRealEvent();
 
               if(alarmDat.alarmStates)
               {
                   if(keyEvent == KEYID_SIDEKEY1 || keyEvent == KEYID_SIDEKEY2 || keyEvent == KEYID_SIDEKEYL1)
                   {
                       keyEvent = Sidekey_GetRemapEvent(keyEvent);
 
                       if(keyEvent == KEYID_SOS)
                       {
                           AlarmFuncSwitch(OFF);
                       }
                   }
                   break;
               }
               
               switch(g_sysRunPara.sysRunMode)
               {
                   case MODE_SLAVE_LISTEN:
                       SlaveListenTask();
                       break;
                   case MODE_MASTER_PAIR:
                       MasterPairTask();
                       break;
                   case MODE_DASHBOARD:
                       HL_KeyProcess_Dashboard(keyEvent);
                       break;
                   case MODE_HL_MENU:
                       HL_KeyProcess_Menu(keyEvent);
                       break;
                   case MODE_DTMF_ANI:
                       HL_KeyProcess_AniContacts(keyEvent);
                       break;
                   case MODE_MENU:
                       if (keyEvent == KEYID_SCAN)
                       {
                           g_sysRunPara.sysRunMode = MODE_MASTER_PAIR;
                           BeepOut(BEEP_FASTSW);
                           MasterPairInit();
                       }
                       else
                       {
                           KeyProcess_Menu(keyEvent);
                       }
                       break;
                   case MODE_FM:
                       KeyProcess_Fm(keyEvent);
                       break;  
                   case MODE_MONI:
                       KeyProcess_Moni(keyEvent);
                       break;   
                   case MODE_SCAN:
                       KeyProcess_Scan(keyEvent);
                       break; 
                   case MODE_SEARCH:
                       KeyProcess_Search(keyEvent);
                       break;  
                   case MODE_SCAN_QT:
                       KeyProcess_ScanQt(keyEvent);
                       break;     
                   case MODE_WEATHER:
                       KeyProcess_Weather(keyEvent);
                       break;  
                   case MODE_STOPWATCH:
                       KeyProcess_StopWatch(keyEvent);
                       break;    
                   case MODE_DTMF:
                       KeyProcess_DtmfInput(keyEvent);
                       break;
                   case MODE_MAIN:
                   default:
                       KeyProcess_Main(keyEvent);
                       break;
               }
           }
           Audio_PlayTask();
           DtmfReceiveTask();
           break;
    }

    if(g_sysRunPara.sysRunMode == MODE_PROGRAM)
    {
        EnterProgromMode();
    }

    if(g_sysRunPara.sysRunMode == MODE_FLASH_PROGRAM)
    {
        EnterFlashProgromMode();
    }
}


