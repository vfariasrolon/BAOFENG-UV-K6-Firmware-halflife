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
            g_sysRunPara.sysRunMode = MODE_HL_MENU;
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
            g_sysRunPara.sysRunMode = MODE_HL_MENU;
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
                    
                    g_sysRunPara.sysRunMode = MODE_MAIN;
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
                            if (g_sysRunPara.sysRunMode == MODE_DASHBOARD)
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
                    if (g_sysRunPara.sysRunMode == MODE_DASHBOARD)
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
void UI_DisplayDashboard(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw terminal-style premium frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Header title inverted retro box
    LCD_DrawRectangle(3, 4, 120, 11, 1);
    LCD_DisplayText(4, 12, (U8 *)"ROIP CEDIS STATUS", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Subheaders
    LCD_DisplayText(18, 6, (U8 *)"ID BATERIA RSSI  ESTADO", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Grid listing of Slaves status
    U8 idx;
    U8 posY = 29;
    for (idx = 0; idx < 3 && idx < g_slaveCount; idx++)
    {
        STR_SLAVE_TELEMETRY *s = &g_slaveTelemetry[idx];
        char lineBuf[30];
        
        // Render Signal Quality as a DBm/RSSI indicator
        int rssiDb = -110 + (s->rssi * 50 / 255);
        if (s->rssi == 255) rssiDb = 0; // unassigned/perfect
        
        sprintf(lineBuf, "%02d  %d%%   -%2ddB  %s", 
                s->id, 
                s->battery, 
                abs(rssiDb),
                s->isOnline ? "ACTIVO" : "PERDIDO");
                
        LCD_DisplayText(posY, 6, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
        posY += 10;
    }
    
    if (g_slaveCount == 0)
    {
        LCD_DisplayText(36, 12, (U8 *)"NO SE DETECTAN", FONTSIZE_12x12, LCD_DIS_NORMAL);
        LCD_DisplayText(46, 12, (U8 *)"DISPOSITIVOS", FONTSIZE_12x12, LCD_DIS_NORMAL);
    }
    
    LCD_UpdateFullScreen();
}

// Slave listening UI Screen
void UI_DisplaySlaveListen(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Outer Industrial Border
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(2, 2, 124, 60, 0);
    
    LCD_DrawRectangle(4, 6, 116, 13, 1);
    LCD_DisplayText(5, 14, (U8 *)"HALF-LIFE OTAP", FONTSIZE_16x16, LCD_DIS_INVERT);
    
    // Show hardware RSSI
    U8 rssiVal = Rfic_GetRssiVal();
    char rssiBuf[20];
    sprintf(rssiBuf, "RSSI: %3d  BAT: %2d%%", rssiVal, BatteryCalculateLevel());
    LCD_DisplayText(22, 12, (U8 *)rssiBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_DisplayText(36, 12, (U8 *)"ESPERANDO MAESTRO", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Premium retro animated scanning dots
    static U8 animationFrame = 0;
    animationFrame = (animationFrame + 1) % 4;
    char progressDots[6];
    memset(progressDots, '.', animationFrame);
    progressDots[animationFrame] = '\0';
    
    LCD_DisplayText(48, 12, (U8 *)"ENLACE ACTIVO", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(48, 92, (U8 *)progressDots, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_UpdateFullScreen();
}

// Master pairing UI Screen
void UI_DisplayMasterPair(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(2, 2, 124, 60, 0);
    
    LCD_DrawRectangle(4, 6, 116, 13, 1);
    LCD_DisplayText(5, 10, (U8 *)"HALFLIFE MAESTRO", FONTSIZE_16x16, LCD_DIS_INVERT);
    
    LCD_DisplayText(24, 12, (U8 *)"PRESIONE PTT PARA", FONTSIZE_12x12, LCD_DIS_NORMAL);
    LCD_DisplayText(36, 12, (U8 *)"OTAP AUTO-ENLACE", FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    char counterBuf[30];
    sprintf(counterBuf, "ENLAZADOS: %02d Slaves", g_slaveCount);
    LCD_DisplayText(48, 12, (U8 *)counterBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    LCD_UpdateFullScreen();
}

// Unified Half-Life Custom Menu Renderer
void UI_DisplayHlMenu(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw premium industrial frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Title header
    LCD_DrawRectangle(3, 4, 120, 11, 1);
    LCD_DisplayText(4, 16, (U8 *)"MENU HALF-LIFE", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Render the 4 items
    const char *menuItems[] = {
        "1. TELEMETRIA",
        "2. MDO MAESTRO",
        "3. MDO ESCLAVO",
        "4. AGENDA DTMF"
    };
    
    U8 idx;
    U8 posY = 18;
    for (idx = 0; idx < 4; idx++)
    {
        if (idx == g_hlMenuIndex)
        {
            // Highlight the selected item with retro inverted box
            LCD_DrawRectangle(posY, 4, 120, 10, 1);
            LCD_DisplayText(posY + 1, 6, (U8 *)menuItems[idx], FONTSIZE_12x12, LCD_DIS_INVERT);
        }
        else
        {
            LCD_DisplayText(posY + 1, 6, (U8 *)menuItems[idx], FONTSIZE_12x12, LCD_DIS_NORMAL);
        }
        posY += 11;
    }
    
    LCD_UpdateFullScreen();
}

// DTMF ANI Contacts list scrollable renderer
void UI_DisplayAniContacts(void)
{
    SC5260_ClearArea(0, 0, 128, 64, 0);
    
    // Draw premium industrial frame
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(1, 1, 126, 62, 0);
    
    // Title header
    LCD_DrawRectangle(3, 4, 120, 11, 1);
    LCD_DisplayText(4, 16, (U8 *)"CONTACTOS DTMF", FONTSIZE_12x12, LCD_DIS_INVERT);
    
    // Draw Own Machine ID
    char ownIdBuf[20];
    char cleanOwnId[6];
    memset(cleanOwnId, 0, sizeof(cleanOwnId));
    U8 i;
    for (i = 0; i < 4 && g_dtmfStore.machineId[i] != '\0' && g_dtmfStore.machineId[i] != 0xFF; i++)
    {
        cleanOwnId[i] = g_dtmfStore.machineId[i];
    }
    sprintf(ownIdBuf, "ID PROPIO: %s", cleanOwnId);
    LCD_DisplayText(18, 6, (U8 *)ownIdBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
    
    // Grid listing of contacts (displaying 3 scrollable entries)
    U8 idx;
    U8 posY = 29;
    
    for (idx = 0; idx < 3; idx++)
    {
        U8 contactIdx = (g_aniContactIndex + idx) % 20;
        STR_CONTACT *c = &dtmfInfo.contact[contactIdx];
        
        char lineBuf[30];
        
        // Clean name (remove any non-printable chars)
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
            sprintf(lineBuf, "%02d. %-6s -> %-4s", contactIdx + 1, cleanName, cleanId);
        }
        
        if (idx == 0) // Currently selected active contact
        {
            LCD_DrawRectangle(posY, 4, 120, 10, 1);
            LCD_DisplayText(posY + 1, 6, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_INVERT);
        }
        else
        {
            LCD_DisplayText(posY + 1, 6, (U8 *)lineBuf, FONTSIZE_12x12, LCD_DIS_NORMAL);
        }
        posY += 11;
    }
    
    LCD_UpdateFullScreen();
}
