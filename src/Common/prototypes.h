#ifndef __PROTOTYPES_H
#define __PROTOTYPES_H

/*
 * prototypes.h — Fase 1: Declaraciones de funciones faltantes
 *
 * Cada prototipo está anotado con su archivo fuente de origen en src_sucio/.
 * Esto permite rastrear la implementación real cuando se integren los módulos.
 *
 * Protocolo Doble Capa:
 *   - Las implementaciones reales están en src_sucio/
 *   - Los stubs están en src/Common/stubs.c
 *   - Este header solo declara — no define
 */

/* ------------------------------------------------------------------ */
/*  Scheduler — src_sucio/src/App/AppTask.h                            */
/* ------------------------------------------------------------------ */
extern void App_10msTask(void);
extern void App_50msTask(void);
extern void App_100msTask(void);
extern void App_500msTask(void);
extern void AppRunTask(void);

/* ------------------------------------------------------------------ */
/*  Board / HAL — src_sucio/src/BSP/Board.h + Board.c                 */
/* ------------------------------------------------------------------ */
extern void Board_Init(void);
extern void Board_Watchdog_Init(void);

/*  LED Driver — src/Driver/led.h                                     */
extern void LED_Init(void);
extern void LED_Toggle(void);

/*  Keyboard Driver — src/Driver/keyboard.h                           */
extern void Keyboard_Init(void);
extern void KEY_ScanTask(void);
extern void ExtraKeys_Init(void);
extern void ExtraKeys_ScanTask(void);

/*  LCD Driver — src/Driver/Sc5260.h                                  */
extern void SPI2_Init(void);
extern void SC5260_Init(void);
extern void SC5260_ClearArea(U8 posY, U8 posX, U8 length, U8 wide, U8 fillData);
extern void LCD_UpdateFullScreen(void);
extern void LCD_RunDiagnosticTest(void);
extern void LCD_ShowAlphabetTest(void);
extern void LCD_DrawIcon(U8 iconID, U8 x, U8 y);
extern void LCD_DrawLogo(void);

/*  Light System — src/Driver/light_system.h                          */
extern void LightSystem_Init(void);
extern void Light_BacklightToggle(void);
extern void Light_LedTopToggle(void);

/* ------------------------------------------------------------------ */
/*  Delay — src_sucio/src/Common/Delay.h + Delay.c                    */
/* ------------------------------------------------------------------ */
extern void DelayMs(U16 n);
extern void DelayUs(U16 n);

/* ------------------------------------------------------------------ */
/*  Beep / Audio — src_sucio/src/Voice/Beep.h + Beep.c                */
/* ------------------------------------------------------------------ */
extern void BeepOut(U8 beepmode);
extern void Audio_PlayVoiceLock(U8 voice);
extern void Audio_PlayTask(void);

/* ------------------------------------------------------------------ */
/*  Keyboard / PTT — src_sucio/src/Driver/keyboard.h                  */
/*                   src_sucio/src/Driver/key_ptt.h                    */
/* ------------------------------------------------------------------ */
extern U8  GetKeyCode(void);
extern U8  Key_GetRealEvent(void);
extern void KEY_ScanTask(void);
extern void PTT_ScanTask(void);
extern void KeyLockFunSwitch(void);

/* ------------------------------------------------------------------ */
/*  RF / Radio Core — src_sucio/src/Core/Radio.h + RadioTask.h        */
/* ------------------------------------------------------------------ */
extern void RF_Task(void);
extern void Radio_TxKeyTone(U8 event, U8 para);
extern void Rfic_Init(void);

/* ------------------------------------------------------------------ */
/*  Radio Config / VFO — src_sucio/src/Core/Functions.h               */
/*                        src_sucio/src/Driver/RadioDataStorage.h      */
/* ------------------------------------------------------------------ */
extern void RadioConfig_Init(void);
extern void RadioVfoInfo_Init(void);
extern void ChannelCheckActiveAll(void);
extern void ChannleVfoDataInit(U8 flagAB, U8 readFlag);
extern void ResetTimeKeyLockAndPowerSave(void);
extern void ResetInputBuf(void);
extern void ResetRadioFunData(void);
extern void ResetVfoModeData(void);
extern void ResetChannelData(void);
extern U16  SeekActiveChannel_Up(U16 curChanNum, U8 isScan);
extern void VfoFrequency2Buf(U32 freq, U8 *dest, U8 len);
extern void CheckSjTimeout(void);
extern void DualStandbyTask(void);

/* ------------------------------------------------------------------ */
/*  Battery — src_sucio/src/App/Battery.h + Battery.c                 */
/* ------------------------------------------------------------------ */
extern void BatteryInitLevel(void);
extern void BatteryCheckTask(void);
extern void CalculateSqlLevel(void);

/* ------------------------------------------------------------------ */
/*  Flash / NorFlash — src_sucio/src/Driver/NorFlash.h                */
/*                      src_sucio/src/Driver/RadioDataStorage.h        */
/* ------------------------------------------------------------------ */
extern void Flash_SaveRadioImfosData(void);
extern void Flash_SaveChannelData(U16 channelNum, U8 *chData, U8 *chName);
extern void Flash_ModifyChannelData(U16 channelNum, U8 *chData, U8 *chName);
extern void Flash_DeleteChannelData(U16 channelNum);
extern void Flash_SaveVfoData(U8 workAB);
extern void Flash_SaveSystemRunData(void);
extern void SpiFlash_ReadBytes(U32 addr, U8 *buf_p, U16 length);

/* ------------------------------------------------------------------ */
/*  CPS / Programación — src_sucio/src/CPS/ProgromFlash.h             */
/* ------------------------------------------------------------------ */
extern void ProgromInit(void);
extern void EnterProgromMode(void);
extern void EnterFlashProgromMode(void);

/* ------------------------------------------------------------------ */
/*  GUI / Display — src_sucio/src/Gui/DisplayPowerOn.h                 */
/*                   src_sucio/src/Gui/DisplayMain.h                   */
/*                   src_sucio/src/Driver/Sc5260.h                     */
/* ------------------------------------------------------------------ */
extern void UI_DisplayPowerOn(void);
extern void DisplaySoftVersion(void);
extern void DisplayHomePage(void);
extern void LCD_BackLightSetOn(void);
extern void LCD_CheckBackLight(void);
extern void SC5260_ClearArea(U8 x, U8 y, U8 w, U8 h, U8 color);

/* ------------------------------------------------------------------ */
/*  App Password — src_sucio/src/Gui/DisplayCheckPassword.h            */
/* ------------------------------------------------------------------ */
extern void App_CheckPowerOnPassword(void);

/* ------------------------------------------------------------------ */
/*  Scan / Moni / DTMF — src_sucio/src/App/AppScan.h                  */
/*                         src_sucio/src/App/AppMoni.h                 */
/*                         src_sucio/src/App/AppDtmf.h                 */
/* ------------------------------------------------------------------ */
extern void ScanTask(void);
extern void CheckExitMenu(void);
extern void CheckPowerOff(void);
extern void CheckAutoKeyLockTask(void);
extern void LightFlashTask(void);
extern void VoxCheckTask(void);
extern void DtmfReceiveTask(void);
extern void KeyProcess_Moni(U8 keyEvent);
extern void KeyProcess_Scan(U8 keyEvent);
extern void KeyProcess_DtmfInput(U8 keyEvent);
extern void KeyProcess_Main(U8 keyEvent);

/* ------------------------------------------------------------------ */
/*  Half-Life — src_sucio/src/App/AppHalfLife.h                        */
/* ------------------------------------------------------------------ */
extern void BackgroundTelemetryTask(void);
extern void HL_SanitizeVfoPointers(void);
extern void SlaveListenTask(void);
extern void MasterPairTask(void);
extern void MasterPairInit(void);
extern void HL_KeyProcess_Dashboard(U8 keyEvent);
extern void HL_KeyProcess_Menu(U8 keyEvent);
extern void HL_KeyProcess_AniContacts(U8 keyEvent);
extern void HL_SetMode(U8 newMode);
extern U8   HL_GetMode(void);

/* ------------------------------------------------------------------ */
/*  Channel check — src_sucio/src/Core/Functions.h                     */
/* ------------------------------------------------------------------ */
extern U8   CheckChannelActive(U16 curChanNum, U8 isScan);
extern U8   CheckFreqInRange(U32 freq);

#endif /* __PROTOTYPES_H */
