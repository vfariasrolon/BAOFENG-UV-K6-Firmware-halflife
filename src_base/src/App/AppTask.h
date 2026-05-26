#ifndef __APPTASK_H
    #define __APPTASK_H

enum WORKMODE_Type
{
    MODE_MAIN = 0,
    MODE_MENU,
    MODE_FM,
    MODE_MONI,
    MODE_SCAN,
    MODE_SEARCH,
    MODE_SCAN_QT,
    MODE_WEATHER,
    MODE_STOPWATCH,
    MODE_DTMF,
    MODE_PROGRAM,
    MODE_FLASH_PROGRAM,
    MODE_SLAVE_LISTEN,
    MODE_MASTER_PAIR,
    MODE_DASHBOARD,
    MODE_HL_MENU,
    MODE_DTMF_ANI
};
/***********************************************************************************************/    

extern void App_10msTask(void);
extern void App_100msTask(void);
extern void App_500msTask(void);
extern void AppRunTask(void);


#endif
