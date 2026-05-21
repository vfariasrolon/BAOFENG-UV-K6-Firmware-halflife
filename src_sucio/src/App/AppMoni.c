#include "includes.h"

extern void EnterMoniMode(void)
{
    ExitAllFunction(0);
    g_sysRunPara.moniFlag = 1;
    HL_SetMode(MODE_MONI);

    // Forzar apertura de canal de audio, receptor y squelch
    Rfic_WakeUp();
    Rfic_TxSingleTone_Off();
    DualStandbyWorkOFF();
    
    LedRxSwitch(LED_ON);
    Rfic_SetAfout(ON);
    SpeakerSwitch(ON);
    g_rfRxState = RX_MONI;

    DisplaySingalFlag(4,1);  
}

void ExitMoniMode(void)
{
    HL_SetMode(MODE_MAIN);
    g_sysRunPara.moniFlag = 0;
    RF_RxEnd();
    SpeakerSwitch(OFF);
    RF_PowerSet(g_ChannelVfoInfo.BandFlag,PWR_OFF); 
    g_rfRxState = RX_READY;
    DisplayHomePage();
}

extern void KeyProcess_Moni(U8 keyEvent)
{
    switch(keyEvent)
    {
        case KEYID_SIDEKEY1:
        case KEYID_SIDEKEY2:
        case KEYID_SIDEKEYL1:
        case KEYID_SIDEKEYL2:
        case KEYID_MONIEXIT:
        case KEYID_LIGHT:
            ExitMoniMode();
            g_sysRunPara.ledState = 0;
            LightSwitch(LED_OFF);
            break;
        default:
            BeepOut(BEEP_NULL);
            break;
    }
}


