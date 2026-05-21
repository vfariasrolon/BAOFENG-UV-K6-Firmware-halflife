#include "AppHalfLife.h"
#include "includes.h"
#include "string.h"
#include "stdlib.h"

// Initialize Global Telemetry and OTAP Variables
STR_SLAVE_TELEMETRY g_slaveTelemetry[MAX_SLAVES];
U8 g_slaveCount = 0;
U8 g_assignedIdCounter = 1;

U8 g_hlMenuIndex = 0;
U8 g_aniContactIndex = 0;

// VRFR Tactical OTAP carrier (Mode A) globals
static U32 s_originalFreqRx = 0;
static U32 s_originalFreqTx = 0;
static U8 s_originalScramble = 0;
static Boolean s_isCurrentlyJumped = FALSE;
static U32 s_jumpInactivityTimer = 0;

// VRFR (A) Pseudorandom Frequency Hopping
static U32 s_xorshiftState = 0;
static U32 s_pendingTacticalFreq = 0;   // Freq chosen, waiting for DTMF to finish to jump master
static Boolean s_masterJumpPending = FALSE;
static Boolean s_dtmfOrderComplete = FALSE;  // Set by DtmfSendTask when DTMF_OVER reached

// Xorshift32 PRNG — fast, small, no division, ideal for Cortex-M0
static U32 VRFR_Xorshift32(void)
{
    s_xorshiftState ^= s_xorshiftState << 13;
    s_xorshiftState ^= s_xorshiftState >> 17;
    s_xorshiftState ^= s_xorshiftState << 5;
    return s_xorshiftState;
}

// Pick a random tactical frequency from 3 bands (in units of 100Hz, as used by BK4829)
// Range 0 — VHF:          136.000 MHz to 174.000 MHz
// Range 1 — UHF Standard: 400.000 MHz to 440.000 MHz
// Range 2 — UHF Tactical: 455.000 MHz to 458.000 MHz
static U32 VRFR_PickRandomFreq(void)
{
    U32 rng = VRFR_Xorshift32();
    U8 band = rng % 3;   // 0, 1 or 2

    // Step in 12.5 kHz steps = 125 units of 100Hz — keeps frequencies clean
    switch (band)
    {
        case 0: // VHF 136.000 - 174.000 MHz  → 1,360,000 to 1,740,000 (units)
        {
            U32 range = (1740000 - 1360000) / 125; // = 3040 steps
            U32 step  = (VRFR_Xorshift32() % range) * 125;
            return 1360000 + step;
        }
        case 1: // UHF 400.000 - 440.000 MHz  → 4,000,000 to 4,400,000
        {
            U32 range = (4400000 - 4000000) / 125; // = 3200 steps
            U32 step  = (VRFR_Xorshift32() % range) * 125;
            return 4000000 + step;
        }
        default: // UHF Tactical 455.000 - 458.000 MHz → 4,550,000 to 4,580,000
        {
            U32 range = (4580000 - 4550000) / 125; // = 240 steps
            U32 step  = (VRFR_Xorshift32() % range) * 125;
            return 4550000 + step;
        }
    }
}

// Custom Procedural Bitmaps for Scrambler Seeds (17x7 pixels)
const U8 iconScr_Seed1[17] = {
    0x7F, 0x41, 0x41, 0x41, 0x5D, 0x55, 0x55, 0x55, 0x5D, 0x55, 0x55, 0x55, 0x5D, 0x41, 0x41, 0x41, 0x7F
};
const U8 iconScr_Seed2[17] = {
    0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x00
};
const U8 iconScr_Seed3[17] = {
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08
};
const U8 iconScr_Seed4[17] = {
    0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55
};

// Internal helper: Convert standard frequency to BCD format stored in K6 SPI Flash
static U32 FrequencyToBCD(U32 freq)
{
    U32 bcd = 0;
    U8 *pBcd = (U8 *)&bcd;
    U32 temp = freq;
    U8 i;
    for (i = 0; i < 4; i++)
    {
        U8 digit_pair = temp % 100;
        pBcd[i] = ((digit_pair / 10) << 4) | (digit_pair % 10);
        temp /= 100;
    }
    return bcd;
}

// Calculate active battery percentage using current ADC readings from raw voltage calibration
extern void BatteryGetLevel(void);
U8 BatteryCalculateLevel(void)
{
    BatteryGetLevel(); // Force reading latest ADC battery values
    U32 curVolt = battery.CurVolt;
    if (curVolt <= 130) return 0;
    if (curVolt >= 183) return 100;
    return (U8)((curVolt - 130) * 100 / (183 - 130));
}

// Save dynamic CEDIS contact dynamically to SPI Flash Channel 0 (default boot channel)
void SaveContactToSPIFlash(U16 chNum, U32 rxFreq, U32 txFreq, U8 *name)
{
    STR_CHANNEL tempCh;
    memset(&tempCh, 0x00, sizeof(STR_CHANNEL));
    
    tempCh.rxFreq = FrequencyToBCD(rxFreq);
    tempCh.txFreq = FrequencyToBCD(txFreq);
    tempCh.rxDCSCTSNum = 0; // No tone
    tempCh.txDCSCTSNum = 0;
    tempCh.dtmfgroup = 1;
    tempCh.pttID = 0;
    tempCh.txPower = 1; // High/Medium Power
    tempCh.chFlag3.Byte = 0;
    tempCh.chFlag3.Bit.b2 = 1;     // Add to scan list
    tempCh.chFlag3.Bit.b6 = 0;     // Wideband
    tempCh.chFlag3.Bit.spMute = 0; // QT Mute
    
    // Name is 12 bytes maximum inside K6 SPI structure
    U8 chName[12];
    memset(chName, 0x20, 12); // Fill with spaces
    U8 nameLen = 0;
    while(name[nameLen] != '\0' && nameLen < 12)
    {
        chName[nameLen] = name[nameLen];
        nameLen++;
    }
    
    Flash_ModifyChannelData(chNum, (U8 *)&tempCh, chName);
}

// Get Silicon Unique Device ID (UUID)
U32 GetSiliconUUID(void)
{
    volatile U32 *uuidReg = (volatile U32 *)(0x1FFFF7E8);
    U32 val = *uuidReg;
    if (val == 0 || val == 0xFFFFFFFF)
    {
        // Fallback to random hash if register is blank
        val = 0xDEADC0DE;
    }
    return val;
}

// Helper: Calculate simple 16-bit sum CRC for FSK packages
static U16 FSK_CalculateChecksum(U16 *pData, U8 len)
{
    U16 sum = 0;
    U8 i;
    for (i = 0; i < len; i++)
    {
        sum += pData[i];
    }
    return sum;
}

// Init Master Pairing mode
void MasterPairInit(void)
{
    g_slaveCount = 0;
    g_assignedIdCounter = 1;
    SC5260_ClearArea(0, 0, 128, 64, 0);
    UI_DisplayMasterPair();
}

// Master triggers Discovery Broadcast (PING)
void MasterPairTrigger(void)
{
    U16 fskPacket[8];
    memset(fskPacket, 0, sizeof(fskPacket));
    
    fskPacket[0] = FSK_CMD_DISCOVER_PING;
    fskPacket[1] = 0; // Broadcast
    fskPacket[2] = 0;
    fskPacket[3] = 0;
    fskPacket[4] = 0;
    fskPacket[5] = 0;
    fskPacket[6] = 0;
    fskPacket[7] = FSK_CalculateChecksum(fskPacket, 7);
    
    // Play Tx beep sound
    BeepOut(BEEP_FASTSW);
    
    // Switch RF hardware to FSK Transmit Mode
    Rfic_EnterFSKMode(1);
    Rfic_FskTransmit(fskPacket);
    Rfic_ExitFSKMode();
    
    // Return BK4819 to normal RX mode
    RxReset();
}

// Master pairing loop task called in 10ms loop when in MODE_MASTER_PAIR
void MasterPairTask(void)
{
    U16 fskPacket[8];
    U8 keyEvent;
    
    // Draw/Refresh pairing UI
    UI_DisplayMasterPair();
    
    // Process keyboard events
    if(g_keyScan.keyEvent != KEYID_NONE)
    {
        keyEvent = Key_GetRealEvent();
        if (keyEvent == KEYID_EXIT)
        {
            // Exit Pairing Mode
            HL_SetMode(MODE_HL_MENU);
            BeepOut(BEEP_EXITMENU);
            extern void UI_DisplayHlMenu(void);
            UI_DisplayHlMenu();
            RxReset();
            return;
        }
    }
    
    // Listen for incoming Slave responses
    Rfic_EnterFSKMode(0); // FSK RX Mode
    if (Rfic_GetFskRxFlag())
    {
        Rfic_ReadFskData(fskPacket);
        
        // Validate checksum
        U16 sum = FSK_CalculateChecksum(fskPacket, 7);
        if (fskPacket[7] == sum && fskPacket[0] == FSK_CMD_DISCOVER_RESP)
        {
            U32 slaveUuid = ((U32)fskPacket[2] << 16) | fskPacket[3];
            U8 slaveBat = fskPacket[4];
            
            // Check if already assigned
            Boolean exists = FALSE;
            U8 i;
            for (i = 0; i < g_slaveCount; i++)
            {
                U32 registeredUuid = ((U32)g_slaveTelemetry[i].id << 16); // or simple check
                if (registeredUuid == slaveUuid)
                {
                    exists = TRUE;
                    break;
                }
            }
            
            if (!exists && g_slaveCount < MAX_SLAVES)
            {
                // Dynamic assignment!
                U8 newId = g_assignedIdCounter++;
                STR_SLAVE_TELEMETRY *s = &g_slaveTelemetry[g_slaveCount];
                s->id = newId;
                s->battery = slaveBat;
                s->rssi = 255; // Placeholder/active
                s->isOnline = TRUE;
                s->lastActiveTime = 0;
                
                // Formulate name SLAVE-X
                sprintf((char *)s->name, "SLV-%02d", newId);
                g_slaveCount++;
                
                // Transmit SET_IDENTITY packet to provision the slave
                U16 setPacket[8];
                memset(setPacket, 0, sizeof(setPacket));
                setPacket[0] = FSK_CMD_SET_IDENTITY;
                setPacket[1] = newId;
                setPacket[2] = (slaveUuid >> 16) & 0xFFFF;
                setPacket[3] = slaveUuid & 0xFFFF;
                setPacket[4] = (s->name[0] << 8) | s->name[1];
                setPacket[5] = (s->name[2] << 8) | s->name[3];
                setPacket[6] = (s->name[4] << 8) | s->name[5];
                setPacket[7] = FSK_CalculateChecksum(setPacket, 7);
                
                // Transmit provision package over the air (OTAP)
                Rfic_EnterFSKMode(1);
                Rfic_FskTransmit(setPacket);
                Rfic_ExitFSKMode();
                
                // Success beep on Master
                BeepOut(BEEP_FASTSW);
                DelayMs(100);
                BeepOut(BEEP_FASTSW);
            }
        }
    }
    Rfic_ExitFSKMode();
}

// Slave listening loop task called in 10ms loop when in MODE_SLAVE_LISTEN
void SlaveListenTask(void)
{
    U16 fskPacket[8];
    U8 keyEvent;
    
    // Refresh Slave Listen Screen
    UI_DisplaySlaveListen();
    
    if(g_keyScan.keyEvent != KEYID_NONE)
    {
        keyEvent = Key_GetRealEvent();
        if (keyEvent == KEYID_EXIT)
        {
            HL_SetMode(MODE_HL_MENU);
            BeepOut(BEEP_EXITMENU);
            extern void UI_DisplayHlMenu(void);
            UI_DisplayHlMenu();
            RxReset();
            return;
        }
    }
    
    // Listen for Master's PING or SET_IDENTITY
    Rfic_EnterFSKMode(0);
    if (Rfic_GetFskRxFlag())
    {
        Rfic_ReadFskData(fskPacket);
        
        U16 sum = FSK_CalculateChecksum(fskPacket, 7);
        if (fskPacket[7] == sum)
        {
            if (fskPacket[0] == FSK_CMD_DISCOVER_PING)
            {
                // Master is calling! Respond with UUID and Battery level
                U32 myUuid = GetSiliconUUID();
                U8 myBat = BatteryCalculateLevel();
                
                U16 respPacket[8];
                memset(respPacket, 0, sizeof(respPacket));
                respPacket[0] = FSK_CMD_DISCOVER_RESP;
                respPacket[1] = 0; // Resp
                respPacket[2] = (myUuid >> 16) & 0xFFFF;
                respPacket[3] = myUuid & 0xFFFF;
                respPacket[4] = myBat;
                respPacket[5] = 0;
                respPacket[6] = 0;
                respPacket[7] = FSK_CalculateChecksum(respPacket, 7);
                
                // Transmit Response
                Rfic_EnterFSKMode(1);
                Rfic_FskTransmit(respPacket);
                Rfic_ExitFSKMode();
                
                BeepOut(BEEP_FMSW2);
            }
            else if (fskPacket[0] == FSK_CMD_SET_IDENTITY)
            {
                U32 myUuid = GetSiliconUUID();
                U32 targetUuid = ((U32)fskPacket[2] << 16) | fskPacket[3];
                
                if (targetUuid == myUuid)
                {
                    // Yes! This is my designated identity!
                    U8 assignedId = fskPacket[1];
                    U8 name[7];
                    name[0] = (fskPacket[4] >> 8) & 0xFF;
                    name[1] = fskPacket[4] & 0xFF;
                    name[2] = (fskPacket[5] >> 8) & 0xFF;
                    name[3] = fskPacket[5] & 0xFF;
                    name[4] = (fskPacket[6] >> 8) & 0xFF;
                    name[5] = fskPacket[6] & 0xFF;
                    name[6] = '\0';
                    
                    // Save dynamically to Channel 0 (default boot channel)
                    // Frequencies are procedurally mapped per ID to keep channel isolation!
                    U32 chFreq = 43000000 + (U32)assignedId * 25000;
                    SaveContactToSPIFlash(0, chFreq, chFreq, name);
                    
                    // Permanent success notification and exit to main!
                    BeepOut(BEEP_FMSW2);
                    DelayMs(100);
                    BeepOut(BEEP_FMSW2);
                    DelayMs(100);
                    BeepOut(BEEP_FMSW2);
                    
                    HL_SetMode(MODE_MAIN);
                    ChannelCheckActiveAll(); // Refresh active channels
                    DisplayHomePage();
                    RxReset();
                    return;
                }
            }
        }
    }
    Rfic_ExitFSKMode();
}

// Heartbeat background telemetry task running periodically in background
void BackgroundTelemetryTask(void)
{
    extern void HL_BackgroundInactivityTask(void);
    HL_BackgroundInactivityTask();

    // VRFR (A): Execute master frequency jump AFTER DTMF order has finished transmitting
    // Triggered by explicit flag set in HL_NotifyDtmfComplete(), called from DtmfSendTask
    if (s_masterJumpPending && s_dtmfOrderComplete)
    {
        s_masterJumpPending   = FALSE;
        s_dtmfOrderComplete   = FALSE;

        // Save original state if not already jumped
        if (!s_isCurrentlyJumped)
        {
            s_originalFreqRx  = g_CurrentVfo->freqRx.frequency;
            s_originalFreqTx  = g_CurrentVfo->freqTx.frequency;
            s_originalScramble = g_CurrentVfo->scarmble;
            s_isCurrentlyJumped = TRUE;
        }

        // Apply the tactical frequency to the active VFO
        g_CurrentVfo->freqRx.frequency = s_pendingTacticalFreq;
        g_CurrentVfo->freqTx.frequency = s_pendingTacticalFreq;
        g_CurrentVfo->scarmble = 0;

        // Retune hardware to new tactical channel
        extern void Rfic_ConfigRxMode(void);
        Rfic_ConfigRxMode();

        s_jumpInactivityTimer = 0;

        // Update dashboard display
        if (HL_GetMode() == MODE_DASHBOARD)
            UI_DisplayDashboard();
    }

    static U32 telemetryTimer = 0;
    telemetryTimer++;
    
    // Run every 10 seconds of active idle loop
    if (telemetryTimer >= 1000) 
    {
        telemetryTimer = 0;
        
        // If SQL is open (busy receiving voice), we skip FSK telemetry to avoid collision!
        if (Audio_CheckBusy()) return;
        
        U16 fskPacket[8];
        memset(fskPacket, 0, sizeof(fskPacket));
        
        // Switch between Master and Slave logic
        if (g_slaveCount > 0)
        {
            // Master: Listen for Slave Heartbeats
            Rfic_EnterFSKMode(0);
            if (Rfic_GetFskRxFlag())
            {
                Rfic_ReadFskData(fskPacket);
                U16 sum = FSK_CalculateChecksum(fskPacket, 7);
                if (fskPacket[7] == sum && fskPacket[0] == FSK_CMD_TELEMETRY_PONG)
                {
                    U8 slaveId = fskPacket[1];
                    U8 slaveBat = fskPacket[2];
                    U8 slaveRssi = fskPacket[3];
                    
                    // Update telemetry entry
                    U8 i;
                    for (i = 0; i < g_slaveCount; i++)
                    {
                        if (g_slaveTelemetry[i].id == slaveId)
                        {
                            g_slaveTelemetry[i].battery = slaveBat;
                            g_slaveTelemetry[i].rssi = slaveRssi;
                            g_slaveTelemetry[i].isOnline = TRUE;
                            g_slaveTelemetry[i].lastActiveTime = 0; // reset timeout
                            if (HL_GetMode() == MODE_DASHBOARD)
                            {
                                UI_DisplayDashboard();
                            }
                            break;
                        }
                    }
                }
            }
            Rfic_ExitFSKMode();
            
            // Increment offline timers for telemetry monitoring
            U8 i;
            for (i = 0; i < g_slaveCount; i++)
            {
                g_slaveTelemetry[i].lastActiveTime++;
                if (g_slaveTelemetry[i].lastActiveTime > 6) // ~60 seconds timeout
                {
                    g_slaveTelemetry[i].isOnline = FALSE;
                    if (HL_GetMode() == MODE_DASHBOARD)
                    {
                        UI_DisplayDashboard();
                    }
                }
            }
        }
        else
        {
            // Slave: Periodic Heartbeat (PONG) transmit
            // Read my assigned ID from Channel 0 Name
            U8 myId = 0;
            U8 chName[12];
            SpiFlash_ReadBytes(0 * CHAN_SIZE + NAME_ADDR_SHIFT, chName, NAME_SIZE);
            if (strncmp((char *)chName, "SLV-", 4) == 0)
            {
                myId = atoi((char *)&chName[4]);
            }
            
            if (myId > 0)
            {
                U8 myBat = BatteryCalculateLevel();
                U8 myRssi = Rfic_GetRssiVal();
                
                fskPacket[0] = FSK_CMD_TELEMETRY_PONG;
                fskPacket[1] = myId;
                fskPacket[2] = myBat;
                fskPacket[3] = myRssi;
                fskPacket[4] = 0;
                fskPacket[5] = 0;
                fskPacket[6] = 0;
                fskPacket[7] = FSK_CalculateChecksum(fskPacket, 7);
                
                Rfic_EnterFSKMode(1);
                Rfic_FskTransmit(fskPacket);
                Rfic_ExitFSKMode();
                
                RxReset();
            }
        }
    }
}

// Gorgeous Industrial Telemetry Dashboard Renderer
static void GetRotatedChar8x16(char ch, U8 *outBuf)
{
    U8 inBuf[16];
    extern void Font_Read_8x16_ASCII( uint8_t *pString, uint8_t *pdat );
    Font_Read_8x16_ASCII((U8 *)&ch, inBuf);
    
    memset(outBuf, 0, 16);
    for (U8 c_rot = 0; c_rot < 16; c_rot++)
    {
        U8 colByte = 0;
        U8 r = c_rot; // original row (0..15) is rotated column (0..15)
        for (U8 r_rot = 0; r_rot < 8; r_rot++)
        {
            U8 c = 7 - r_rot; // original column (0..7)
            U8 bit;
            if (r < 8)
            {
                bit = (inBuf[c] >> r) & 1;
            }
            else
            {
                bit = (inBuf[8 + c] >> (r - 8)) & 1;
            }
            colByte |= (bit << r_rot);
        }
        outBuf[c_rot] = colByte;
    }
}

static void DrawHalfLifeBranding(void)
{
    // Draw solid black block on the left (Y=2..62, X=2..23)
    SC5260_ClearArea(2, 2, 22, 60, 1);
    
    // Draw dividing line at X=24
    SC5260_ClearArea(2, 24, 1, 60, 1);
    
    // Draw rotated "H A L F" in white (inverted)
    U8 rotBuf[16];
    
    // 'H' at Y = 47, X = 4 (Top of the screen due to inverted Y page layout)
    GetRotatedChar8x16('H', rotBuf);
    SC5260_DisplayArea(47, 4, 16, 8, rotBuf, 1);
    
    // 'A' at Y = 34, X = 4
    GetRotatedChar8x16('A', rotBuf);
    SC5260_DisplayArea(34, 4, 16, 8, rotBuf, 1);
    
    // 'L' at Y = 21, X = 4
    GetRotatedChar8x16('L', rotBuf);
    SC5260_DisplayArea(21, 4, 16, 8, rotBuf, 1);
    
    // 'F' at Y = 8, X = 4 (Bottom of the screen due to inverted Y page layout)
    GetRotatedChar8x16('F', rotBuf);
    SC5260_DisplayArea(8, 4, 16, 8, rotBuf, 1);
}

void UI_DisplayDashboard(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw outer frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Draw left-side branding
    DrawHalfLifeBranding();
    
    // Title on the right
    LCD_DrawRectangle(3, 26, 98, 11, 1);
    LCD_DisplayText(4, 50, (U8 *)"VRFR (A)", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Subheaders
    LCD_DisplayText(18, 27, (U8 *)"ID BAT  RSSI  ST", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Grid listing of Slaves status
    U8 idx;
    U8 posY = 29;
    for (idx = 0; idx < 3 && idx < g_slaveCount; idx++)
    {
        STR_SLAVE_TELEMETRY *s = &g_slaveTelemetry[idx];
        char lineBuf[32];
        
        int rssiDb = -110 + (s->rssi * 50 / 255);
        if (s->rssi == 255) rssiDb = 0;
        
        sprintf(lineBuf, "%02d %3d%% -%2ddB %s", 
                s->id, 
                s->battery, 
                abs(rssiDb),
                s->isOnline ? "OK" : "KO");
                
        LCD_DisplayText(posY, 27, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
        posY += 10;
    }
    
    if (g_slaveCount == 0)
    {
        LCD_DisplayText(32, 42, (U8 *)"SIN EQUIPOS", FONTSIZE_12x12, LCD_DIS_NORMAL);
        LCD_DisplayText(44, 45, (U8 *)"DETECTADOS", FONTSIZE_12x12, LCD_DIS_NORMAL);
    }
    
    // Draw tactical alert overlay if transmitting in VRFR Mode A
    extern U8 g_rfState;
    if (g_rfState == 2)
    {
        SC5260_ClearArea(2, 26, 98, 44, 1); // clear content area (black background)
        LCD_DrawRectangle(2, 26, 98, 44, 1); // white outline inside black block
        
        LCD_DisplayText(4, 30, (U8 *)"TRANSMITIENDO", FONTSIZE_12x12, LCD_DIS_INVERT);
        LCD_DisplayText(20, 32, (U8 *)" ORDEN VRFR A", FONTSIZE_12x12, LCD_DIS_INVERT);
        
        char freqBuf[16];
        sprintf(freqBuf, "%d.%03d MHz", (int)(g_CurrentVfo->freqRx.frequency / 100000), (int)((g_CurrentVfo->freqRx.frequency / 100) % 1000));
        LCD_DisplayText(34, 32, (U8 *)freqBuf, FONTSIZE_12x12, LCD_DIS_INVERT);
        
        LCD_DisplayText(47, 30, (U8 *)" MICRO ACTIVO", FONTSIZE_12x12, LCD_DIS_INVERT);
    }
    // Draw tactical alert overlay if temporarily jumped in VRFR Mode A
    else if (s_isCurrentlyJumped)
    {
        SC5260_ClearArea(2, 26, 98, 44, 1); // clear content area (black background)
        LCD_DrawRectangle(2, 26, 98, 44, 1); // white outline inside black block
        
        LCD_DisplayText(4, 30, (U8 *)" ALERTA VRFR ", FONTSIZE_12x12, LCD_DIS_INVERT);
        
        char freqBuf[16];
        // Format frequency as XXX.XXX
        sprintf(freqBuf, "%d.%03d MHz", (int)(g_CurrentVfo->freqRx.frequency / 100000), (int)((g_CurrentVfo->freqRx.frequency / 100) % 1000));
        LCD_DisplayText(20, 32, (U8 *)freqBuf, FONTSIZE_12x12, LCD_DIS_INVERT);
        
        char scrBuf[16];
        sprintf(scrBuf, "SCRAMBLER:%02d", g_CurrentVfo->scarmble);
        LCD_DisplayText(34, 32, (U8 *)scrBuf, FONTSIZE_12x12, LCD_DIS_INVERT);
        
        LCD_DisplayText(47, 30, (U8 *)" ENLACE ACTIVO", FONTSIZE_12x12, LCD_DIS_INVERT);
    }
    
    LCD_UpdateFullScreen();
}

void UI_DisplaySlaveListen(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw outer frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Draw left-side branding
    DrawHalfLifeBranding();
    
    // Title on the right
    LCD_DrawRectangle(4, 26, 98, 11, 1);
    LCD_DisplayText(5, 54, (U8 *)"ESCLAVO", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Show hardware RSSI
    U8 rssiVal = Rfic_GetRssiVal();
    char rssiBuf[24];
    sprintf(rssiBuf, "RSSI:%3d B:%2d%%", rssiVal, BatteryCalculateLevel());
    LCD_DisplayText(20, 27, (U8 *)rssiBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_DisplayText(34, 39, (U8 *)"ESPERANDO M.", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_DisplayText(48, 42, (U8 *)"BUSCANDO...", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_UpdateFullScreen();
}

void UI_DisplayMasterPair(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw outer frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Draw left-side branding
    DrawHalfLifeBranding();
    
    // Title on the right
    LCD_DrawRectangle(4, 26, 98, 11, 1);
    LCD_DisplayText(5, 54, (U8 *)"MAESTRO", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    LCD_DisplayText(20, 36, (U8 *)"PRESIONAR PTT", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(34, 39, (U8 *)"PARA ENLAZAR", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    char counterBuf[32];
    sprintf(counterBuf, "ENLAZADOS: %02d", g_slaveCount);
    LCD_DisplayText(48, 33, (U8 *)counterBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_UpdateFullScreen();
}

void UI_DisplayHlMenu(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw outer frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Draw left-side branding
    DrawHalfLifeBranding();
    
    // Render the items
    const char *menuItems[] = {
        "1. VRFR (A)",
        "2. MDO MAESTRO",
        "3. MDO ESCLAVO",
        "4. AGENDA DTMF"
    };
    
    U8 start_idx = 0;
    // Sliding window of 3 visible items
    if (g_hlMenuIndex >= 3) {
        start_idx = g_hlMenuIndex - 2;
    } else if (g_hlMenuIndex >= 2) {
        start_idx = 1;
    } else {
        start_idx = 0;
    }
    
    U8 i;
    for (i = 0; i < 3; i++)
    {
        U8 item_idx = start_idx + i;
        if (item_idx >= 4) break;
        
        U8 drawY = 8 + (i * 18);
        U8 isSelected = (item_idx == g_hlMenuIndex);
        
        if (isSelected)
        {
            // Highlight box matching layout
            LCD_DrawRectangle(drawY - 1, 26, 98, 14, 1);
            LCD_DisplayText(drawY + 1, 28, (U8 *)menuItems[item_idx], FONTSIZE_12x12, LCD_DIS_INVERT);
        }
        else
        {
            LCD_DisplayText(drawY + 1, 28, (U8 *)menuItems[item_idx], FONTSIZE_12x12, LCD_DIS_NORMAL);
        }
    }
    
    LCD_UpdateFullScreen();
}

void UI_DisplayAniContacts(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw outer frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Draw left-side branding
    DrawHalfLifeBranding();
    
    // Title on the right
    LCD_DrawRectangle(3, 26, 98, 11, 1);
    LCD_DisplayText(4, 42, (U8 *)"AGENDA DTMF", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Draw Own Machine ID
    char ownIdBuf[24];
    char cleanOwnId[6];
    memset(cleanOwnId, 0, sizeof(cleanOwnId));
    U8 i;
    for (i = 0; i < 4 && g_dtmfStore.machineId[i] != '\0' && g_dtmfStore.machineId[i] != 0xFF; i++)
    {
        cleanOwnId[i] = g_dtmfStore.machineId[i];
    }
    sprintf(ownIdBuf, "PROPIO ID: %s", cleanOwnId);
    LCD_DisplayText(18, 30, (U8 *)ownIdBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Grid listing of contacts
    U8 idx;
    U8 posY = 29;
    
    for (idx = 0; idx < 3; idx++)
    {
        U8 contactIdx = (g_aniContactIndex + idx) % 20;
        STR_CONTACT *c = &dtmfInfo.contact[contactIdx];
        
        char lineBuf[32];
        
        // Clean name
        char cleanName[12];
        memset(cleanName, 0, sizeof(cleanName));
        for (i = 0; i < 10 && c->name[i] != '\0' && c->name[i] != 0xFF; i++)
        {
            cleanName[i] = c->name[i];
        }
        
        // Clean ID
        char cleanId[6];
        memset(cleanId, 0, sizeof(cleanId));
        for (i = 0; i < 4 && c->id[i] != '\0' && c->id[i] != 0xFF; i++)
        {
            cleanId[i] = c->id[i];
        }
        
        if (cleanName[0] == '\0' || cleanName[0] == ' ')
        {
            sprintf(lineBuf, "%02d. [VACIO]", contactIdx + 1);
        }
        else
        {
            sprintf(lineBuf, "%02d.%s->%s", contactIdx + 1, cleanName, cleanId);
        }
        
        if (idx == 0) // Selected
        {
            LCD_DrawRectangle(posY, 26, 98, 10, 1);
            LCD_DisplayText(posY + 1, 28, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_INVERT);
        }
        else
        {
            LCD_DisplayText(posY + 1, 28, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
        }
        posY += 11;
    }
    
    LCD_UpdateFullScreen();
}


// --- VRFR Tactical OTAP carrier (Mode A) ---


void HL_RestoreOriginalChannel(void)
{
    if (s_isCurrentlyJumped)
    {
        g_CurrentVfo->freqRx.frequency = s_originalFreqRx;
        g_CurrentVfo->freqTx.frequency = s_originalFreqTx;
        g_CurrentVfo->scarmble = s_originalScramble;
        s_isCurrentlyJumped = FALSE;
        
        extern void Rfic_ConfigRxMode(void);
        extern void Rfic_SetScramble(U8 group, U32 freq);
        Rfic_ConfigRxMode();
        Rfic_SetScramble(g_CurrentVfo->scarmble, g_CurrentVfo->freqRx.frequency);
        
        BeepOut(BEEP_EXITMENU);
        
        // Redraw screen if in main menus
        if (HL_GetMode() == MODE_DASHBOARD)
        {
            UI_DisplayDashboard();
        }
    }
}

void HL_BackgroundInactivityTask(void)
{
    if (s_isCurrentlyJumped)
    {
        // If squelch is open (receiving carrier) or radio is transmitting, refresh the timer!
        extern U8 g_rfState;
        if (g_sysRunPara.rfRxFlag.rxReceiveOn == ON || g_rfState == 2) // RF_TX is constant 2
        {
            s_jumpInactivityTimer = 0;
        }
        else
        {
            s_jumpInactivityTimer++;
            // 500 * 10ms = 5 seconds hang time before auto-restoring
            if (s_jumpInactivityTimer >= 500)
            {
                HL_RestoreOriginalChannel();
            }
        }
    }
}

void HL_ProcessIncomingOTAP(const char *dtmfString)
{
    // Format validation
    if (dtmfString[0] == 'A' && dtmfString[11] == '#')
    {
        // Extract frequency
        U32 decodedFreq = 0;
        U8 i;
        for (i = 3; i <= 8; i++)
        {
            decodedFreq = decodedFreq * 10 + (dtmfString[i] - '0');
        }
        decodedFreq *= 100; // to Hz
        
        // Extract Scrambler
        U8 decodedScramble = (dtmfString[9] - '0') * 10 + (dtmfString[10] - '0');
        if (decodedScramble > 4) decodedScramble = 0;
        
        // If decodedFreq is 0, this is a CLOSE/Restore command!
        if (decodedFreq == 0)
        {
            HL_RestoreOriginalChannel();
            return;
        }
        
        // Save original state if not already jumped
        if (!s_isCurrentlyJumped)
        {
            s_originalFreqRx = g_CurrentVfo->freqRx.frequency;
            s_originalFreqTx = g_CurrentVfo->freqTx.frequency;
            s_originalScramble = g_CurrentVfo->scarmble;
            s_isCurrentlyJumped = TRUE;
        }
        
        // Update active VFO
        g_CurrentVfo->freqRx.frequency = decodedFreq;
        g_CurrentVfo->freqTx.frequency = decodedFreq;
        g_CurrentVfo->scarmble = decodedScramble;
        
        // Sintonizar hardware
        extern void Rfic_ConfigRxMode(void);
        extern void Rfic_SetScramble(U8 group, U32 freq);
        Rfic_ConfigRxMode();
        Rfic_SetScramble(g_CurrentVfo->scarmble, g_CurrentVfo->freqRx.frequency);
        
        s_jumpInactivityTimer = 0;
        
        // Military sounding warning beep (double alert!)
        BeepOut(BEEP_LOWBAT);
        
        // Display alert on screen if on custom menus
        if (HL_GetMode() == MODE_DASHBOARD)
        {
            UI_DisplayDashboard();
        }
    }
}

void HL_TxVrfrModeA(U8 flagClose)
{
    U8 seed = 9; // Cell ID 09 (default master)
    U32 txFreq;

    memset(g_sysRunPara.txDtmfCode.code, 0xFF, 16);

    // Digit 0: 'A' -> 10
    g_sysRunPara.txDtmfCode.code[0] = 10;

    // Digit 1-2: Seed
    g_sysRunPara.txDtmfCode.code[1] = seed / 10;
    g_sysRunPara.txDtmfCode.code[2] = seed % 10;

    if (flagClose)
    {
        // CLOSE command: freq = 000000, scramble = 00 → slaves & master return home
        U8 i;
        for (i = 3; i <= 8; i++)
            g_sysRunPara.txDtmfCode.code[i] = 0;
        g_sysRunPara.txDtmfCode.code[9]  = 0;
        g_sysRunPara.txDtmfCode.code[10] = 0;
    }
    else
    {
        // OPEN: seed Xorshift with SysTick hardware VAL register (free-running 24-bit counter)
        // XOR'd with machineId for per-radio uniqueness at PTT press moment
        s_xorshiftState = SysTick->VAL ^ ((U32)g_dtmfStore.machineId[0] << 24)
                                       ^ ((U32)g_dtmfStore.machineId[1] << 16)
                                       ^ ((U32)g_dtmfStore.machineId[2] << 8)
                                       ^ (U32)g_dtmfStore.machineId[3];
        if (s_xorshiftState == 0) s_xorshiftState = 0xDEADBEEF; // Xorshift must not be 0

        // Pick random tactical frequency from 3 bands
        txFreq = VRFR_PickRandomFreq();
        s_pendingTacticalFreq = txFreq;  // Save so master can jump after DTMF
        s_masterJumpPending   = TRUE;

        // Encode frequency into DTMF digits 3-8 (in units of 100Hz)
        U32 f = txFreq;
        U8 i;
        for (i = 8; i >= 3; i--)
        {
            g_sysRunPara.txDtmfCode.code[i] = f % 10;
            f /= 10;
        }
        // Scramble stays at 00 for random hops (hardware scramble not needed)
        g_sysRunPara.txDtmfCode.code[9]  = 0;
        g_sysRunPara.txDtmfCode.code[10] = 0;
    }

    // Digit 11: '#' -> 15
    g_sysRunPara.txDtmfCode.code[11] = 15;
    g_sysRunPara.txDtmfCode.codeLen  = 12;

    // Queue async DTMF — transmits on ORIGINAL channel, master jumps in BackgroundTelemetryTask
    extern void DtmfSendCodeOn(U8 type);
    DtmfSendCodeOn(DTMF_TYPEIN);
}

extern void HL_Hook_OnPttPress(void)
{
    if (HL_GetMode() == MODE_DASHBOARD)
    {
        HL_TxVrfrModeA(0); // OPEN
    }
}

extern void HL_Hook_OnPttRelease(void)
{
    if (HL_GetMode() == MODE_DASHBOARD)
    {
        HL_TxVrfrModeA(1); // CLOSE
    }
}

void HL_SanitizeVfoPointers(void)
{
    if (g_CurrentVfo)
    {
        if (g_CurrentVfo->rx == NULL || (U32)g_CurrentVfo->rx < 0x20000000 || (U32)g_CurrentVfo->rx > 0x20006000)
        {
            g_CurrentVfo->rx = &g_CurrentVfo->freqRx;
        }
        if (g_CurrentVfo->tx == NULL || (U32)g_CurrentVfo->tx < 0x20000000 || (U32)g_CurrentVfo->tx > 0x20006000)
        {
            g_CurrentVfo->tx = &g_CurrentVfo->freqTx;
        }
    }
}

void HL_KeyProcess_Dashboard(U8 keyEvent)
{
    if (keyEvent == KEYID_EXIT)
    {
        HL_SetMode(MODE_HL_MENU);
        BeepOut(BEEP_EXITMENU);
        UI_DisplayHlMenu();
    }
    else
    {
        UI_DisplayDashboard();
    }
}

void HL_KeyProcess_Menu(U8 keyEvent)
{
    if (keyEvent == KEYID_EXIT)
    {
        HL_SetMode(MODE_MAIN);
        BeepOut(BEEP_EXITMENU);
        extern void DisplayHomePage(void);
        DisplayHomePage();
    }
    else if (keyEvent == KEYID_UP)
    {
        if (g_hlMenuIndex > 0) g_hlMenuIndex--;
        else g_hlMenuIndex = 3;
        BeepOut(BEEP_FASTSW);
        UI_DisplayHlMenu();
    }
    else if (keyEvent == KEYID_DOWN)
    {
        if (g_hlMenuIndex < 3) g_hlMenuIndex++;
        else g_hlMenuIndex = 0;
        BeepOut(BEEP_FASTSW);
        UI_DisplayHlMenu();
    }
    else if (keyEvent == KEYID_MENU)
    {
        BeepOut(BEEP_FASTSW);
        if (g_hlMenuIndex == 0)
        {
            HL_SetMode(MODE_DASHBOARD);
            UI_DisplayDashboard();
        }
        else if (g_hlMenuIndex == 1)
        {
            HL_SetMode(MODE_MASTER_PAIR);
            MasterPairInit();
        }
        else if (g_hlMenuIndex == 2)
        {
            HL_SetMode(MODE_SLAVE_LISTEN);
            UI_DisplaySlaveListen();
        }
        else if (g_hlMenuIndex == 3)
        {
            HL_SetMode(MODE_DTMF_ANI);
            g_aniContactIndex = 0;
            UI_DisplayAniContacts();
        }
    }
}

void HL_KeyProcess_AniContacts(U8 keyEvent)
{
    if (keyEvent == KEYID_EXIT)
    {
        HL_SetMode(MODE_HL_MENU);
        BeepOut(BEEP_EXITMENU);
        UI_DisplayHlMenu();
    }
    else if (keyEvent == KEYID_UP)
    {
        if (g_aniContactIndex > 0) g_aniContactIndex--;
        else g_aniContactIndex = 19;
        BeepOut(BEEP_FASTSW);
        UI_DisplayAniContacts();
    }
    else if (keyEvent == KEYID_DOWN)
    {
        if (g_aniContactIndex < 19) g_aniContactIndex++;
        else g_aniContactIndex = 0;
        BeepOut(BEEP_FASTSW);
        UI_DisplayAniContacts();
    }
}



void HL_SetMode(U8 newMode)
{
    // Sanitize state transition: keep within valid boundaries (0..16)
    if (newMode <= 16)
    {
        g_sysRunPara.sysRunMode = newMode;
    }
    else
    {
        g_sysRunPara.sysRunMode = 0; // MODE_MAIN
    }
}

U8 HL_GetMode(void)
{
    if (g_sysRunPara.sysRunMode > 16)
    {
        g_sysRunPara.sysRunMode = 0;
    }
    return g_sysRunPara.sysRunMode;
}

void HL_NotifyDtmfComplete(void)
{
    // Called by DtmfSendTask the moment a DTMF sequence finishes.
    // Signals BackgroundTelemetryTask to execute the master frequency jump.
    if (s_masterJumpPending)
    {
        s_dtmfOrderComplete = TRUE;
    }
}
