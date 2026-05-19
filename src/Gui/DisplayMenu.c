#include "includes.h"

#define EnS                           "abc"
#define EnL                           "ABC"
#define Num                           "123"
#define Pinyin                        " PY"

String disBuf[17];

const STR_MENU_ITEM MenuList[] = 
{
    {vo_Null,   "CHL Name","NOM CANAL"},
    {vo_Null,   "RX Frequency","FREQ REC" },
    {vo_Null,   "TX Frequency","FREQ TRANS"},
    {vo_CTCSS,  "R-CTCSS","R-CTCSS"},
    {vo_DCS,    "R-DCS","R-DCS"},
    {vo_CTCSS,  "T-CTCSS","T-CTCSS"},
    {vo_DCS,    "T-DCS","T-DCS"},
    {vo_Chlbandwidth,"BandWidth","ANCHO BANDA"},
    {vo_power,  "TXP","POTENCIA TX"},
    {vo_Null,   "Silence Mode" ,"SILENCIO" },
    {vo_Null,   "TX Forbid","BLOQUEO TX"},
    {vo_Freqdir,"Shift DIR","DIR DESV"},
    {vo_Offsetfreq,"OFFSET","DESVIACION"},
    {vo_Memorychl,"MEMCH","MEM CANAL"},
    {vo_Deletechl,"DELCH","BORRAR CANAL"},
    {vo_Null,    "VFO FreqRang","RANGO VFO"},
    {vo_Null,    "Scan Mode","MDO ESCAN"},
    {vo_Null,    "DTMFST","DTMFST"},
    {vo_Null,    "PTT ID","PTT ID"},
    {vo_Null,    "PTT-LT","RETARDO PTT" },
    {vo_Squelch, "Squelch","SILENCIADOR"},
    {vo_savemode,"RX Save","AHORRO RX"},
    {vo_VOX,     "VOX Switch","VOX"},
    {vo_Null,    "VOX Level","NIVEL VOX"},
    {vo_Null,    "VOX Delay","RETARDO VOX"},
    {vo_Txovertime,"TX OVer Time","LIMITE TX"},
    {vo_Voicepromrt,"Language","IDIOMA"},
    {vo_Null,    "VOICE","VOZ PANT"},
    {vo_Null,    "Menu HangTime","PERM MENU"},
    {vo_Beepprompt,"BEEP PROMPT","BEEP TECLADO"},
    {vo_Null,    "Roger Beep","BEEP ROGER"},
    {vo_Null,    "POWER ON TYPE","MODO ENC"},
    {vo_Null,    "Power On Tone","TONO ENC"},
    {vo_Null,    "Power on MSG","MSG ENC"},
    {vo_Null,    "POWER ON PWD","PASS ENC"},
    {vo_Dualstandby,"Dual Watch","DOBLE ESCU"},
    {vo_Null,    "MDF-A","PANT A"},
    {vo_Null,    "MDF-B","PANT B"},
    {vo_Null,    "RP-STE","RP-STE"},
    {vo_Null,    "RPT-RL","RPT-RL"},
    {vo_Null,    "ALERT","ALERTA" },
    {vo_Step,    "Freq Step","PASO FREQ"},
    {vo_Busylockout,"Busy Lockout","BLOQ OCUP"},
    {vo_Null,    "Side Tone","TONO LATERAL"},
    {vo_Null,    "Alarm Mode","MODO ALARMA"},
    {vo_Null,    "PF1","TECLA PF1"},
    {vo_Null,    "PF1 LONG PRESS","PF1 LARGO" },
    {vo_Null,    "PF2", "TECLA PF2"},
    {vo_Null,    "ABR","LUM PANT"},
    {vo_Null,    "Brightness","BRILLO"},
    {vo_Null,    "LCD Reflex","INV PANT"},
    {vo_Null,    "AUTOLOCK","BLOQ AUT"},
    {vo_Null,    "Radio Interrupt","INT RADIO"},
    {vo_initialization,"Reset","RESET"},
    {vo_Null,    "STOP WATCH","CRONOMETRO"},
    {vo_Null,    "VERSION","VERSION"},
    {vo_Null,    "MIC Gain","GANANCIA MIC"},
};

const STR_MENU_ITEM MenuFmList[] = 
{
    {vo_Null,    "FM MEMCH","MEM CANAL FM"},
    {vo_Null,    "FM BAND", "BANDA FM"},
};

const String *VoxCnStr[] = 
{
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10"
};

const String *BatSaveCnStr[] = 
{
    "APAGADO",
    "1:1",
    "1:2",
    "1:4"
};
const String *BatSaveEnStr[] = 
{
    "OFF",
    "1:1",
    "1:2",
    "1:4",
};

const String *OnOffCnStr[] = 
{
    "APAGADO",
    "ENCENDIDO"
};
const String *OnOffEnStr[] = 
{
    "OFF",
    "ON"
};

const String *OnSelCnStr[] = 
{
    "ENCENDIDO"
};
const String *OnSelEnStr[] = 
{
    "ON"
};

const String *AlmodCnStr[] = 
{
    "LOCAL",
    "SONIDO",
    "CODIGO"
};
const String *AlmodEnStr[] = 
{
    "ON SITE",
    "SEND SOUND",
    "SEND CODE"
};

const String *ScanmodCnStr[] = 
{
    "TO",
    "CO",
    "SE"
};
const String *ScanmodEnStr[] = 
{
    "TO",
    "CO",
    "SE"
};

const String *PttIdSelCnStr[] = 
{
    "APAGADO",
    "INICIO",
    "FINAL",
    "AMBOS"
};
const String *PttIdSelEnStr[] = 
{
    "OFF",
    "BOT" ,
    "EOT",
    "BOTH"
};

const String *BandCnStr[] = 
{
    "ANCHO",
    "ESTRECHO"
};
const String *BandEnStr[] = 
{
    "WIDE",
    "NARROW",
};

const String *TxPowerCnStr[] = 
{
    "ALTO",
    "BAJO"
};
const String *TxPowerEnStr[] = 
{
    "HIGH",
    "LOW"
};

const String *VfoStepStr[] = 
{
    "2.5 K",
    "5.0 K",
    "6.25 K",
    "10.0 K",
    "12.5 K",
    "20.0 K",
    "25.0 K",
    "50.0 K",
};

const String *VfoDirCnStr[] = 
{
    "APAGADO",
    "+",
    "-"
};

const String *VfoDirEnStr[] = 
{
    "OFF",
    "+",
    "-"
};

const String *ChDisCnStr[] = 
{
    "NOMBRE",
    "FREQ",
    "CANAL",
    "NOM+FREQ"
};

const String *ChDisEnStr[] = 
{
    "NAME",
    "FREQ",
    "CH",    
    "NAME+FREQ"
};

const String *LanStr[] = 
{
    "ENGLISH",
    "ESPANOL"
};

const String *DtmfSetSelCnStr[] = 
{
    "APAGADO",
    "DT-ST",
    "ANI-ST",
    "DT+ANI"
};
const String *DtmfSetSelEnStr[] = 
{
    "OFF",
    "DT-ST",
    "ANI-ST",
    "DT+ANI"
};

const String *DevResetCnStr[] = 
{
    "VFO",
    "TODO"
};

const String *DevResetEnStr[] = 
{
    "VFO",
    "All"
};

const String *ToneEnStr[] = 
{
    " 1000hz",//0
    " 1450hz",//1
    " 1750hz",//2
    " 2100hz",//3
};

const String *PwrOnCnStr[] = 
{
    "LOGO",
    "MENSAJE",
    "VOLTAJE"
};
const String *PwrOnEnStr[] = 
{
    "LOGO",
    "MESSAGE",
    "VOLTAGE"
};

const String *SideKeyCnStr[] = 
{
    "NINGUNA",
    "LINTERNA",
    "POTENCIA",
    "ESCANER",
    "VOX",
    "ALARMA",
    "RADIO FM"
};
const String *SideKeyEnStr[] = 
{
    "None",
    "Torch On/Off",
    "Power Select ",
    "Scan On/Off",
    "VOX On/Off",
    "Alarm on/off",
    "Radio on/off",
};

const String *RxEndTailSelCnStr[] = 
{
    "APAGADO",
    "MDC1200"
};

const String *RxEndTailSelEnStr[] = 
{
    "OFF",
    "MDC1200"
};

const String *TxEndToneCnStr[] =
{
    "APAGADO",
    "BEEP",
    "TONE1200"
};

const String *TxEndToneEnStr[] =
{
    "OFF",
    "BEEP",
    "TONE1200"
};

const String *ReflexCnStr[] = 
{
    "NORMAL",
    "INVERTIDO"
};

const String *ReflexEnStr[] = 
{
    "Normal",
    "Reflex"
};

const String *PwrOnToneSelEnStr[] = 
{
    "None",
    "Tone",
    "Voice"
};

const String *PwrOnToneSelCnStr[] = 
{
    "NINGUNO",
    "TONO",
    "VOZ"
};

const String *SpMuteSelCnStr[] = 
{
    "CTDCS",
    "CTDCS+SE"
};

const String *SpMuteSelEnStr[] = 
{
    "CTDCS",
    "CTDCS+Signaling",
};

const String *DualSelCnStr[] = 
{
    "APAGADO",
    "DOBLE ESP",
    "ESPERA SIG"
};

const String *DualSelEnStr[] = 
{
    "OFF",
    "Double Wait",
    "Signal Wait"
};

const String *FMBandItemStr[] = 
{
    "76-108Mhz",
    "65-76Mhz",
};

const U8 PttIDDelay[] = {0,1,2,4,6,8,10};

extern void Menu_GetSubItemString(U8 menuIndex)
{   
    if(g_menuInfo.menuType == 1)
    {
        switch(menuIndex)
        {
            case 0:
                sprintf(disBuf,"%02d",g_menuInfo.selectedItem+1); 
                break;
            default:
                break;
        }
    }
    else
    {
        switch(menuIndex)
        {
            case S_MICGAIN:
            case S_ROGERVOL:
            case S_SQVOL:
                sprintf(disBuf, "%d", g_menuInfo.selectedItem);
                break;
            case S_TXTEST:
                sprintf(disBuf, "M%d", g_menuInfo.selectedItem);
                break;
            case S_KEYBEEP:
                sprintf(disBuf, "%s", g_menuInfo.selectedItem ? "ON" : "OFF");
                break;
            default:
                break;
        }
    }    
}

extern void Menu_DisplayFreqError(void)
{
    String disBuf[17];
    
    TranStrToMiddle(disBuf, (String *)"out of range!",16);
    LCD_DisplayText(47, 0, (U8 *)disBuf, FONTSIZE_16x16, 0);
    LCD_UpdateWorkAre();

    //延时1S显示保存成功界面
    DelaySysMs(500);
}

void DisplayInputOffect(U32 selItem)
{
    U8 i,j;
    String buf[10]={0};

    selItem = g_menuInfo.inputVal / 10;
    if(g_menuInfo.isSubMenu == 0)
    {
        sprintf(disBuf,"%d.%04d",selItem/10000,selItem%10000);
    }
    else
    {
        if(g_menuInfo.inputVal)
        {
            sprintf(disBuf,"%d.%04d",selItem/10000,selItem%10000);
        }
        else
        {
            memset(buf,'-',7);
            buf[2] = '.';
    
            j = 0;
            for(i=0;i<g_inputbuf.len;i++)
            {
                if(i == 2)
                {
                   j++;
                }
                buf[j++] = g_inputbuf.buf[i];
            }
            buf[7] = 0;
            strncpy(disBuf,buf,16);
        }
    }
}
void DisplayInputChFreq(U32 selItem)
{
    U8 i,j;
    String buf[10]={0};

    selItem = g_menuInfo.inputVal;
    if(g_menuInfo.isSubMenu == 0)
    {
        sprintf(disBuf,"%d.%05d",selItem/100000,selItem%100000);
    }
    else
    {
        if(g_menuInfo.inputVal)
        {
            sprintf(disBuf,"%d.%05d",selItem/100000,selItem%100000);
        }
        else
        {
            memset(buf,'-',7);
            buf[3] = '.';
    
            j = 0;
            for(i=0;i<g_inputbuf.len;i++)
            {
                if(i == 3)
                {
                   j++;
                }
                buf[j++] = g_inputbuf.buf[i];
            }
            buf[7] = 0;
            strncpy(disBuf,buf,16);
        }
    }
}
void DisplayInputVfoScan(U32 selItem)
{
    U8 i,j;
    String buf[10]={0};

    selItem = g_menuInfo.inputVal;
    if(g_menuInfo.isSubMenu == 0)
    {
        sprintf(disBuf,"%03d-%03d",selItem/1000,selItem%1000);
    }
    else
    {
        if(g_menuInfo.inputVal)
        {
            sprintf(disBuf,"%03d-%03d",selItem/1000,selItem%1000);
        }
        else
        {
            memset(buf,'-',7);
    
            j = 0;
            for(i=0;i<g_inputbuf.len;i++)
            {
                if(i == 3)
                {
                   j++;
                }
                buf[j++] = g_inputbuf.buf[i];
            }
            buf[7] = 0;
            strncpy(disBuf,buf,16);
        }
    }
}

static U8 CheckCtcssInList(U16 ctcssDat)
{
   U8 i;

   if(ctcssDat == 0)
   {
       return 0;
   }
   
   //判断亚音频是否在索引�?
   for(i=0;i<51;i++)
   {
       if(CTCS_TAB[i] == ctcssDat)
       {
           return (i);
       }
   }

   return 0xFF;
}

void GetCtcssDisBuf(U16 Index)
{    
    if(Index == 0)
    {
        sprintf(disBuf,"OFF");
    }
    else if(Index == 0xFF)
    {
        sprintf(disBuf,"%d.%dHz",g_menuInfo.inputVal/10,g_menuInfo.inputVal%10);
    }
    else
    {
        sprintf(disBuf,"%d.%dHz",CTCS_TAB[Index]/10,CTCS_TAB[Index]%10);
    }
}


static void ShowCtcssList(void)
{
    U8 selecteId;

    if(g_menuInfo.isSubMenu == 0)
    {//在�??�?级�?�择菜单选项

       if(g_menuInfo.inputVal == 0)
       {
           selecteId = 0;
       }
       else
       {
           selecteId = CheckCtcssInList(g_menuInfo.inputVal);
       }
       GetCtcssDisBuf(selecteId);
    }
    else
    {
        selecteId = CheckCtcssInList(g_menuInfo.inputVal);
        //selecteId = g_menuInfo.selectedItem;
        GetCtcssDisBuf(selecteId);
    }
}

extern void UpdateMenuDisplay(void)
{
    U8 i;
    char textBuf[32];
    U8 start_idx = 0;

    
    // Clear the work area (Y from 13 to 63, X from 0 to 127)
    SC5260_ClearArea(13, 0, 128, 51, 0);
    
    // Draw the vertical separating line at X=24
    SC5260_ClearArea(13, 24, 1, 51, 1);
    
    // Draw vertical text "MENU" on the left column (X=9)
    LCD_DisplayText(16, 9, (U8 *)"M", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(27, 9, (U8 *)"E", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(38, 9, (U8 *)"N", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(49, 9, (U8 *)"U", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Sliding window of 3 visible items for our 5 calibration options (from index 0 to 4)
    if (g_menuInfo.menuIndex >= 3) {
        start_idx = g_menuInfo.menuIndex - 2;
    } else if (g_menuInfo.menuIndex >= 2) {
        start_idx = 1;
    } else {
        start_idx = 0;
    }
    
    // Draw the 3 visible menu items
    for (i = 0; i < 3; i++) {
        U8 item_idx = start_idx + i;
        U8 drawY = 16 + (i * 16);
        U8 isSelected = (item_idx == g_menuInfo.menuIndex);
        U8 invertMode = (isSelected && g_menuInfo.isSubMenu) ? LCD_DIS_INVERT : LCD_DIS_NORMAL;
        
        // Build the display string: "Name: Value"
        if (item_idx == S_MICGAIN) {
            U8 val = g_radioInform.remain0[0];
            if (val > 31) val = 26;
            if (isSelected && g_menuInfo.isSubMenu) {
                val = g_menuInfo.selectedItem;
            }
            sprintf(textBuf, "MIC GAIN:%d", val);
        }
        else if (item_idx == S_ROGERVOL) {
            U8 val = g_radioInform.remain0[1];
            if (val == 0) val = 224;
            if (isSelected && g_menuInfo.isSubMenu) {
                val = g_menuInfo.selectedItem;
            }
            sprintf(textBuf, "ROGER VOL:%d", val);
        }
        else if (item_idx == S_SQVOL) {
            U8 val = g_radioInform.remain0[2];
            if (val == 0) val = 120;
            if (isSelected && g_menuInfo.isSubMenu) {
                val = g_menuInfo.selectedItem;
            }
            sprintf(textBuf, "SQL LEVEL:%d", val);
        }
        else if (item_idx == S_TXTEST) {
            U8 val = g_radioInform.remain0[3];
            if (val > 4) val = 1;
            if (isSelected && g_menuInfo.isSubMenu) {
                val = g_menuInfo.selectedItem;
            }
            if (val == 0) {
                sprintf(textBuf, "ROGER BEEP:OFF");
            } else {
                sprintf(textBuf, "ROGER BEEP:P%d", val);
            }
        }
        else if (item_idx == S_KEYBEEP) {
            U8 val = g_radioInform.beepsSwitch;
            if (isSelected && g_menuInfo.isSubMenu) {
                val = g_menuInfo.selectedItem;
            }
            sprintf(textBuf, "KEY BEEP:%s", val ? "ON" : "OFF");
        }
        
        // Draw the bullet dot if the item is selected
        if (isSelected) {
            SC5260_ClearArea(drawY + 4, 28, 4, 4, 1);
        }
        
        // Display the text at X=36
        LCD_DisplayText(drawY, 36, (U8 *)textBuf, FONTSIZE_12x12, invertMode);
    }
    
    // Draw status bar icons
    DisplayStateBar();
    LCD_UpdateWorkAre();
}

extern void DisplayInputType(void)
{
    String *str;
    String buf[6];

    if(inputTypeBack != g_inputbuf.inputType)
    {
        inputTypeBack = g_inputbuf.inputType;
        switch(g_inputbuf.inputType)
        {
            case IN_EN_L:
                str = EnS;
                break;
            case IN_EN_U:
                str = EnL;
                break;
            case IN_PINYIN:
                str = Pinyin;
                break;
            case IN_NUMBER:
                default:
                str = Num;
                break;
        }
        sprintf(buf,"%s",str);
        SC5260_ClearArea(10,107,20,9,1);
        //显示输入类型
        LCD_DisplayNumber(11,108,(U8  *)buf,1);
        LCD_UpdateWorkAre();
    }
}

const U8  InputPosY[] = {13,32};

void DisplaySelectHz(void)
{
    U8 i;
    String displayBuf[19];
    
    for(i=0;i<2;i++)
    {
        if(lcdDispBuf[i][0] == 0x08)
        {//�?�?�?义字符时不显示，会�?�致总长�?+1
            sprintf(displayBuf,"%-*.*s",17,17,lcdDispBuf[i]);
            displayBuf[16+1] = 0;
        }
        else
        {
            sprintf(displayBuf,"%-*.*s",16,16,lcdDispBuf[i]);
            displayBuf[16] = 0;
        }
        SC5260_ClearArea(30, 2, 124, 1, 1);
        LCD_DisplayText(InputPosY[i],4,(U8  *)displayBuf,FONTSIZE_16x16,0);
    }
}

extern void MenuShowInputChar(void)
{
    DrowInputWindow(); 

    DisplayInputType();

    //显示输入的字符串
    sprintf((String *)disBuf,"%*.*s",16,16,g_inputbuf.buf);
    //显示内�?�在右下�?
    LCD_DisplayText(47,0,(U8  *)disBuf,FONTSIZE_16x16,0);
    uartSendChar(disBuf[0]);

    /*绘制汉字输入�?*/
    if(pyBuf.pos != 0)
    {
        DisplaySelectHz();
    }
    
    inputTypeBack = 0xFF;
    DisplayInputType();
}

extern void Menu_Display(void)
{
    DisplayBattaryFlag(0);
    
    switch(g_menuInfo.inputMode)
    {
        case MENU_ONE_CHAR:
            if(g_menuInfo.isSubMenu)
            {
                LCD_ClearWorkArea();
                MenuShowInputChar();
                return;
            }
            else
            {
                Menu_GetSubItemString(g_menuInfo.menuIndex);
            }
           break;
        case MENU_ONE_FREQ:
            if(g_menuInfo.isSubMenu == 0)
            {
                g_menuInfo.inputVal = g_menuInfo.selectedItem;
            }
            DisplayInputOffect(g_menuInfo.inputVal);
            break;
        case MENU_CH_FREQ:
            if(g_menuInfo.isSubMenu == 0)
            {
                g_menuInfo.inputVal = g_menuInfo.selectedItem;
            }
            DisplayInputChFreq(g_menuInfo.inputVal);
            break;    
        case MENU_ONE_VFOSCAN:
            if(g_menuInfo.isSubMenu == 0)
            {
                g_menuInfo.inputVal = g_menuInfo.selectedItem;
            }
            DisplayInputVfoScan(g_menuInfo.inputVal);
            break;
        case MENU_ONE_CTCSS:
            ShowCtcssList();
            break;
        case MENU_ONE_DIGIT:
            sprintf(disBuf,"%d",g_menuInfo.selectedItem);
            break;
        case MENU_ONE_SELECT:
        default:
            Menu_GetSubItemString(g_menuInfo.menuIndex);
            break;
    }
    UpdateMenuDisplay();
}
