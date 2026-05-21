#ifndef __DISPLAYMENU_H
    #define __DISPLAYMENU_H

#define MENU_MAX_CNT 5


typedef struct
{
    uint8_t voiceId;
    const char nameEn[16];  //英文名称
    const char nameCn[16];  //�?文名�?
}STR_MENU_ITEM;

//definicion reducida
enum{   
    S_SQL=0,
    S_TXPR,
    S_MICGAIN,
    S_ROGERVOL,
    S_ROGE
};
extern String disBuf[17];
/********************************************************************************************************************/
extern const STR_MENU_ITEM g_MenuList[MENU_MAX_CNT];
extern U8 inputTypeBack;
extern void DisplayInputType(void);
extern void MenuShowInputChar(void);
extern void Menu_GetSubItemString(U8 menuIndex);
extern void Menu_DisplayFreqError(void);
extern void Menu_Display(void);

#endif
