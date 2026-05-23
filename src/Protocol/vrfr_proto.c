#include "vrfr_proto.h"
#include "../App/AppEventManager.h"
#include "../Driver/Sc5260.h"
#include "../Driver/minifont.h"
#include "../Driver/BK4829_Minimal.h"
#include "../Core/TimeManager.h"
#include "../Core/AudioEngine.h"
#include "../Driver/light_system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t g_fsk_current_cfg = 0; // Index 0-7

const FSK_TestConfig_t g_fsk_test_configs[8] = {
    {0xAA30, 16, false}, // 1: Len 16, CRC OFF, Wait BIT0
    {0xAA30, 16, true},  // 2: Len 16, CRC OFF, Drain Manual
    {0x5665, 16, false}, // 3: Len 16, CRC ON, Wait BIT0
    {0x5665, 16, true},  // 4: Len 16, CRC ON, Drain Manual
    {0xAA30, 12, false}, // 5: Len 12, CRC OFF, Wait BIT0
    {0xAA30, 12, true},  // 6: Len 12, CRC OFF, Drain Manual
    {0x5665, 12, false}, // 7: Len 12, CRC ON, Wait BIT0
    {0x5665, 12, true}   // 8: Len 12, CRC ON, Drain Manual
};

uint32_t g_remoteCounter = 0;
char g_lastRxData[32] = {0};
char g_lastTxData[32] = {0};
char g_statusMsg[16] = "IDLE";
char g_txRxState[16] = "[RX] IDLE";

static uint32_t s_randomSeed = 12345;

/* Motor Pseudoaleatorio Xorshift32 */
uint32_t VRFR_Xorshift32(uint32_t *state) {
    uint32_t x = *state;
    if (x == 0) x = 0xDEADBEEF; 
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

// ==========================================
// RENDERIZADO UI (TEST BENCH)
// ==========================================
void VRFR_RenderDiagnostics(void) {
    if (g_uiState != UI_STATE_TEST_BENCH) return;

    SC5260_ClearArea(0, 0, 128, 64, 0); // Clear background
    
    // Línea 1: TX/RX Status + Configuración FSK
    char stateBuf[32];
    snprintf(stateBuf, sizeof(stateBuf), "%s [CFG: %d]", g_txRxState, g_fsk_current_cfg + 1);
    UI_DrawText(0, 16, stateBuf, SCALE_NORMAL);
    
    // Línea 2: DTMF Sent/Received
    char dtmfBuf[32];
    snprintf(dtmfBuf, sizeof(dtmfBuf), "DTMF: %s", g_lastTxData[0] ? g_lastTxData : (g_lastRxData[0] ? g_lastRxData : "---"));
    UI_DrawText(0, 32, dtmfBuf, SCALE_NORMAL);
    
    // Línea 3: Status/Audio
    char audioBuf[32];
    snprintf(audioBuf, sizeof(audioBuf), "AUDIO: %s", g_statusMsg);
    UI_DrawText(0, 48, audioBuf, SCALE_NORMAL);
    
    LCD_UpdateFullScreen();
}

// ==========================================
// RECEPCIÓN (RX FSM)
// ==========================================
static char s_rxCmd[3];
static char s_rxData[32];
static char s_rxCrc;

static uint8_t CalcCRC(const char* cmd, const char* data) {
    uint32_t sum = 0;
    while (*cmd) sum += *cmd++;
    while (*data) sum += *data++;
    return (sum % 10) + '0';
}

static void VRFR_ProcessPayload(void) {
    // Verificar CRC
    char expectedCrc = CalcCRC(s_rxCmd, s_rxData);
    if (expectedCrc != s_rxCrc) {
        strcpy(g_statusMsg, "CRC ERR");
        VRFR_RenderDiagnostics();
        return;
    }

    strcpy(g_lastRxData, s_rxData);

    if (strcmp(s_rxCmd, VRFR_CMD_HANDSHAKE) == 0) {
        uint32_t receivedRand = atoi(s_rxData);
        char response[16];
        snprintf(response, sizeof(response), "%lu", (unsigned long)(receivedRand + 1));
        VRFR_SendPayload(VRFR_CMD_ACK, response);
        strcpy(g_statusMsg, "HNDSHK OK");
        AudioEngine_PlayAck();
    } 
    else if (strcmp(s_rxCmd, VRFR_CMD_LIGHT_OFF) == 0) {
        LightSystem_Set(0);
        VRFR_SendPayload(VRFR_CMD_ACK, "0");
        strcpy(g_statusMsg, "LIG OFF");
        AudioEngine_PlayAck();
    } 
    else if (strcmp(s_rxCmd, VRFR_CMD_LIGHT_ON) == 0) {
        LightSystem_Set(1);
        VRFR_SendPayload(VRFR_CMD_ACK, "0");
        strcpy(g_statusMsg, "LIG ON");
        AudioEngine_PlayAck();
    } 
    else if (strcmp(s_rxCmd, VRFR_CMD_CNT) == 0) {
        g_remoteCounter++;
        char cntStr[16];
        snprintf(cntStr, sizeof(cntStr), "%lu", (unsigned long)g_remoteCounter);
        VRFR_SendPayload(VRFR_CMD_ACK, cntStr);
        strcpy(g_statusMsg, "CNT++");
        AudioEngine_PlayAck();
    }
    else if (strcmp(s_rxCmd, VRFR_CMD_ACK) == 0) {
        strcpy(g_statusMsg, "ACK OK");
        AudioEngine_PlayAck();
    }

    VRFR_RenderDiagnostics();
}


// ==========================================
// ESTADOS DE TRANSMISIÓN Y TIMEOUT
// ==========================================
typedef enum {
    TX_STATE_IDLE,
    TX_STATE_WAIT_ACK,
    TX_STATE_ERROR
} VRFR_TxState_Enum;

static VRFR_TxState_Enum s_txState = TX_STATE_IDLE;
static uint32_t s_txWaitStart = 0;
static uint8_t s_txRetries = 0;
static char s_txLastCmd[3] = {0};
static char s_txLastData[32] = {0};

void VRFR_SendPayload(const char* cmd, const char* data) {
    char packet[64] = {0};
    
    strcpy(s_txLastCmd, cmd);
    strcpy(s_txLastData, data);
    
    strcpy(g_statusMsg, "TXING...");
    strcpy(g_txRxState, "[TX] FSK");
    snprintf(g_lastTxData, sizeof(g_lastTxData), "%s:%s", cmd, data);
    g_lastRxData[0] = '\0'; // Limpiar rx display
    VRFR_RenderDiagnostics();
    
    // Formato FSK: CMD + DATA + CRC
    int pIdx = snprintf(packet, sizeof(packet), "%s%s%c", cmd, data, CalcCRC(cmd, data));
    
    uint8_t target_len = g_fsk_test_configs[g_fsk_current_cfg].payload_len;
    
    // Rellenar con ceros hasta alcanzar exactamente el tamaño objetivo
    for (int i = pIdx; i < target_len; i++) {
        packet[i] = 0;
    }
    
    // Transmitir en bloque
    BK4829_SendFSKData((const uint8_t*)packet, target_len);
    
    strcpy(g_txRxState, "[RX] IDLE");
    VRFR_RenderDiagnostics();
    
    // Si no es un ACK, esperamos un ACK de vuelta
    if (strcmp(cmd, VRFR_CMD_ACK) != 0) {
        s_txState = TX_STATE_WAIT_ACK;
        s_txWaitStart = g_SystemTick;
    } else {
        s_txState = TX_STATE_IDLE;
    }
}

void VRFR_Tick(void) {
    // 1. Manejo de Timeout de TX
    if (s_txState == TX_STATE_WAIT_ACK) {
        if ((g_SystemTick - s_txWaitStart) > 500) { // 500ms timeout
            if (s_txRetries < 2) {
                s_txRetries++;
                strcpy(g_statusMsg, "RETRYING");
                VRFR_RenderDiagnostics();
                // Reenviar
                VRFR_SendPayload(s_txLastCmd, s_txLastData);
                // (VRFR_SendPayload resetea s_txWaitStart)
            } else {
                s_txState = TX_STATE_ERROR;
                strcpy(g_statusMsg, "ERR");
                s_txRetries = 0;
                VRFR_RenderDiagnostics();
            }
        }
    }

    // 2. Recepción de FSK
    uint8_t fskBuf[64] = {0};
    uint8_t len = BK4829_GetFSKData(fskBuf);
    
    if (len >= 3) { // min length: CMD (2) + CRC (1)
        // El último byte es CRC
        s_rxCrc = fskBuf[len - 1];
        
        // Extraer CMD (2 bytes)
        s_rxCmd[0] = fskBuf[0];
        s_rxCmd[1] = fskBuf[1];
        s_rxCmd[2] = '\0';
        
        // Extraer Data
        uint8_t dataLen = len - 3;
        if (dataLen > 0 && dataLen < sizeof(s_rxData)) {
            memcpy(s_rxData, &fskBuf[2], dataLen);
            s_rxData[dataLen] = '\0';
        } else {
            s_rxData[0] = '\0';
        }
        
        VRFR_ProcessPayload();
        
        // Limpiar estado de TX si llegó una respuesta válida
        if (strcmp(g_lastRxData, VRFR_CMD_ACK) == 0 || strstr(g_statusMsg, "ACK OK") != NULL) {
            s_txState = TX_STATE_IDLE;
            s_txRetries = 0;
        }
    }
}

void VRFR_ProcessLocalKey(uint8_t key) {
    if (key >= 1 && key <= 8) {
        // Seleccionar configuración de matriz (1 al 8)
        g_fsk_current_cfg = key - 1;
        
        // Reiniciar módem con la nueva configuración
        BK4829_PrepareFSKReceive();
        
        // Actualizar UI
        VRFR_RenderDiagnostics();
    } else if (key == 0) {
        // Usar tecla 0 (o PTT) para enviar payload de prueba
        VRFR_SendPayload("SNC", "12345678");
    }
}

void VRFR_Init(void) {
    s_randomSeed = g_SystemTick + 123;
    
    // Preparar el módem FSK para escuchar inmediatamente al arrancar
    BK4829_PrepareFSKReceive();
}
