/*
 * stubs.c — Fase 1: Heartbeat Minimalista
 *
 * Stubs vacíos de todas las funciones externas requeridas por App_10msTask()
 * para que el proyecto compile sin los módulos de hardware reales.
 *
 * IMPORTANTE: Estos stubs son SOLO para validar el scheduler y el IWDG loop.
 * Deben ser reemplazados por las implementaciones reales una vez que los
 * módulos de Driver/ y Common/ sean recuperados del proyecto original.
 */

#include "includes.h"

/* ------------------------------------------------------------------ */
/*  App_10msTask dependencies                                          */
/* ------------------------------------------------------------------ */

void RF_Task(void)
{
    /* stub — BK4829 RF driver no disponible en Fase 1 */
}

void KEY_ScanTask(void)
{
    /* stub — KeyScan driver no disponible en Fase 1 */
}

void PTT_ScanTask(void)
{
    /* stub — PTT driver no disponible en Fase 1 */
}

void BackgroundTelemetryTask(void)
{
    /* stub — Half-Life telemetry no disponible en Fase 1 */
}

/* ------------------------------------------------------------------ */
/*  App_50msTask dependencies                                          */
/* ------------------------------------------------------------------ */

void ScanTask(void)              { /* stub */ }
void DualStandbyTask(void)       { /* stub */ }
void LCD_CheckBackLight(void)    { /* stub */ }

/* ------------------------------------------------------------------ */
/*  App_100msTask dependencies                                         */
/* ------------------------------------------------------------------ */

void CheckExitMenu(void)         { /* stub */ }
void CheckPowerOff(void)         { /* stub */ }
void CheckAutoKeyLockTask(void)  { /* stub */ }
void LightFlashTask(void)        { /* stub */ }
void VoxCheckTask(void)          { /* stub */ }

/* ------------------------------------------------------------------ */
/*  App_500msTask dependencies                                         */
/* ------------------------------------------------------------------ */

void BatteryCheckTask(void)      { /* stub */ }
void CalculateSqlLevel(void)     { /* stub */ }
void CheckSjTimeout(void)        { /* stub */ }

/* ------------------------------------------------------------------ */
/*  AppRunTask / main() dependencies críticas                          */
/* ------------------------------------------------------------------ */

void Board_Init(void)            { /* stub */ }
void RadioConfig_Init(void)      { /* stub */ }
void UI_DisplayPowerOn(void)     { /* stub */ }
void Rfic_Init(void)             { /* stub */ }
void ChannelCheckActiveAll(void) { /* stub */ }
void BatteryInitLevel(void)      { /* stub */ }
void App_CheckPowerOnPassword(void) { /* stub */ }
void DisplaySoftVersion(void)    { /* stub */ }
void RadioVfoInfo_Init(void)     { /* stub */ }
void ResetTimeKeyLockAndPowerSave(void) { /* stub */ }
void ResetInputBuf(void)         { /* stub */ }
void ProgromInit(void)           { /* stub */ }
void LCD_BackLightSetOn(void)    { /* stub */ }
void HL_SanitizeVfoPointers(void){ /* stub */ }
void SlaveListenTask(void)       { /* stub */ }
void Audio_PlayTask(void)        { /* stub */ }
void MasterPairTask(void)        { /* stub */ }
void DtmfReceiveTask(void)       { /* stub */ }
void EnterProgromMode(void)      { /* stub */ }
void EnterFlashProgromMode(void) { /* stub */ }
void MasterPairInit(void)        { /* stub */ }

/* GetKeyCode devuelve KEYID_NONE para que main() no entre en los bloques de boot */
U8 GetKeyCode(void)              { return KEYID_NONE; }

/* Key / event stubs */
U8 Key_GetRealEvent(void)        { return KEYID_NONE; }

/* Beep stub — silencioso */
void BeepOut(U8 beepType)        { (void)beepType; }

/* LCD stubs */
void SC5260_ClearArea(U8 x, U8 y, U8 w, U8 h, U8 color)
    { (void)x; (void)y; (void)w; (void)h; (void)color; }
void LCD_DisplayText(U8 x, U8 y, U8 *text, U8 fontSize, U8 mode)
    { (void)x; (void)y; (void)text; (void)fontSize; (void)mode; }

/* Half-Life mode stubs */
void HL_SetMode(U8 mode)         { (void)mode; }
U8   HL_GetMode(void)            { return MODE_MAIN; }

/* Radio TX stubs */
void Radio_TxKeyTone(U8 key, U8 para) { (void)key; (void)para; }

/* KeyProcess stubs */
void KeyProcess_Main(U8 key)     { (void)key; }
void KeyProcess_Menu(U8 key)     { (void)key; }
void KeyProcess_Moni(U8 key)     { (void)key; }
void KeyProcess_Scan(U8 key)     { (void)key; }
void KeyProcess_DtmfInput(U8 key){ (void)key; }
void HL_KeyProcess_Dashboard(U8 key)    { (void)key; }
void HL_KeyProcess_Menu(U8 key)         { (void)key; }
void HL_KeyProcess_AniContacts(U8 key)  { (void)key; }
void Audio_PlayVoiceLock(U8 voice)      { (void)voice; }

/* ------------------------------------------------------------------ */
/*  Definiciones de variables globales                                  */
/*  Declaradas como extern en stub_constants.h                          */
/* ------------------------------------------------------------------ */

volatile U8         g_10msFlag  = FALSE;
volatile U8         g_50msFlag  = FALSE;
volatile U8         g_100msFlag = FALSE;
volatile U8         g_500msFlag = FALSE;

U8                  g_rfState   = RF_RX;
U8                  g_rfTxState = TX_READY;
U8                  g_rfRxState = RX_READY;

STR_SCAN_INFO       g_scanInfo  = { SCAN_IDLE };

STR_RADIO_INFORM    g_radioInform = {
    .language     = LANG_EN,
    .menuExitTime = 5,
    .txPower      = 1,
};

STR_KEYSCAN         g_keyScan   = {
    .keyEvent = KEYID_NONE,
    .keyPara  = 0,
};

