#ifndef __APP_HALFLIFE_H
#define __APP_HALFLIFE_H

#include "includes.h"

// FSK Commands for CEDIS Logistics Ecosystem
#define FSK_CMD_DISCOVER_PING     0x5A01
#define FSK_CMD_DISCOVER_RESP     0x5A02
#define FSK_CMD_SET_IDENTITY      0x5A03
#define FSK_CMD_TELEMETRY_PING    0x5A04
#define FSK_CMD_TELEMETRY_PONG    0x5A05

// Default provisioning channel (430.000 MHz)
#define OTAP_PROVISION_FREQ       43000000

// Max Slaves monitored in CEDIS flotilla
#define MAX_SLAVES                32

// Slaves Telemetry Status
typedef struct {
    U8 id;
    U8 battery;
    U8 rssi;
    U32 lastActiveTime; // ticks
    Boolean isOnline;
    U8 name[7];
} STR_SLAVE_TELEMETRY;

extern STR_SLAVE_TELEMETRY g_slaveTelemetry[MAX_SLAVES];
extern U8 g_slaveCount;
extern U8 g_assignedIdCounter;

// Custom Bitmaps for Procedural Scrambler Icons
extern const U8 iconScr_Seed1[17];
extern const U8 iconScr_Seed2[17];
extern const U8 iconScr_Seed3[17];
extern const U8 iconScr_Seed4[17];

// Functions for OTAP and Telemetry
void SaveContactToSPIFlash(U16 chNum, U32 rxFreq, U32 txFreq, U8 *name);
void MasterPairInit(void);
void MasterPairTrigger(void);
void MasterPairTask(void);
void SlaveListenTask(void);
void BackgroundTelemetryTask(void);
void UI_DrawProceduralIcon(U8 posY, U8 posX, U8 seed);
void UI_DisplayDashboard(void);
void UI_DisplaySlaveListen(void);
void UI_DisplayMasterPair(void);
void UI_DisplayHlMenu(void);
void UI_DisplayAniContacts(void);
U32 GetSiliconUUID(void);

void HL_TxVrfrModeA(U8 flagClose);
void HL_ProcessIncomingOTAP(const char *dtmfString);

extern U8 g_hlMenuIndex;
extern U8 g_aniContactIndex;

#endif
