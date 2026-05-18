#include "includes.h"

// Stubs para AppFm.c (Radio FM)
STR_FMSTATE fmInfo;

Boolean CheckFmChActive(U8 curChanNum)
{
    return FALSE;
}

void ResumeFmMode(void)
{
}

void FmBandConfig(void)
{
}

Boolean CheckFmVfoMode(void)
{
    return FALSE;
}

void FmCheckChannelActive(void)
{
}

void FmCheckTimeOut(void)
{
}

void FmEnterSleepMode(void)
{
}

void FMSwitchExit(void)
{
}

void EnterFmMode(void)
{
}

void ExitFmMode(void)
{
}

void FmTaskFunc(void)
{
}

void ResetFmSleepTime(void)
{
}

void KeyProcess_Fm(U8 keyEvent)
{
}

// Stubs para AppAlarm.c (Alarmas)
STR_ALARM alarmDat;

void CheckAlarmDelay(void)
{
}

Boolean SetAlarmCode(void)
{
    return FALSE;
}

void AlarmTask(void)
{
}

void AlarmFuncSwitch(ENUM_ONOFF flag)
{
}

// Stubs para AppWeather.c (Sistema Meteorológico)
STR_FREQINFO Weatherfreq;
const U32 TAB_WEATHER[1] = {0};

void WeatherInit(U8 num)
{
}

void EnterWeatherMode(void)
{
}

void ExitWeatherMode(void)
{
}

U32 Weather_GetChFreq(U8 num)
{
    return 0;
}

void WeatherScanNextChannel(void)
{
}

void WeatherScanTask(void)
{
}

void KeyProcess_Weather(U8 keyEvent)
{
}
