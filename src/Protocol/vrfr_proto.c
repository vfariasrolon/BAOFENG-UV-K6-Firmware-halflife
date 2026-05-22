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
    
    // Línea 1: TX/RX Status
    UI_DrawText(0, 16, g_txRxState, SCALE_NORMAL);
    
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
typedef enum {
    RX_STATE_IDLE,
    RX_STATE_CMD1,
    RX_STATE_CMD2,
    RX_STATE_DATA,
    RX_STATE_CRC
} VRFR_RxState_Enum;

static VRFR_RxState_Enum s_rxState = RX_STATE_IDLE;
static char s_rxCmd[3];
static char s_rxData[32];
static uint8_t s_rxDataIdx = 0;
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

void VRFR_HandleIncomingTone(char tone) {
    if (tone == 'A') { // START_TOKEN
        s_rxState = RX_STATE_CMD1;
        s_rxDataIdx = 0;
        memset(s_rxData, 0, sizeof(s_rxData));
        return;
    }

    switch (s_rxState) {
        case RX_STATE_IDLE:
            break;
        case RX_STATE_CMD1:
            s_rxCmd[0] = tone;
            s_rxState = RX_STATE_CMD2;
            break;
        case RX_STATE_CMD2:
            s_rxCmd[1] = tone;
            s_rxCmd[2] = '\0';
            s_rxState = RX_STATE_DATA;
            break;
        case RX_STATE_DATA:
            if (tone == 'D') { // END_TOKEN for empty payload
                s_rxCrc = '0'; // default
                VRFR_ProcessPayload();
                s_rxState = RX_STATE_IDLE;
            } else if (tone == 'C') {
                // Separator, ignore
            } else {
                s_rxData[s_rxDataIdx++] = tone;
                // Asumimos que el último caracter antes de D es CRC
                // Esto se maneja viendo cuándo llega D
            }
            break;
        case RX_STATE_CRC:
            break;
    }
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
    int pIdx = 0;
    
    strcpy(s_txLastCmd, cmd);
    strcpy(s_txLastData, data);
    
    strcpy(g_statusMsg, "TXING...");
    strcpy(g_txRxState, "[TX] HOP+");
    snprintf(g_lastTxData, sizeof(g_lastTxData), "%s:%s", cmd, data);
    g_lastRxData[0] = '\0'; // Limpiar rx display
    VRFR_RenderDiagnostics();
    
    // Formato: A CMD DATA CRC D
    packet[pIdx++] = 'A';
    packet[pIdx++] = cmd[0];
    packet[pIdx++] = cmd[1];
    
    // Copiar DATA e inyectar 'C' cada 4 dígitos
    int chunk = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        if (chunk == 4) {
            packet[pIdx++] = 'C';
            chunk = 0;
        }
        packet[pIdx++] = data[i];
        chunk++;
    }
    
    // Checksum
    packet[pIdx++] = CalcCRC(cmd, data);
    packet[pIdx++] = 'D';
    packet[pIdx] = '\0';
    
    BK4829_SendDTMFStringRF(packet);
    
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

// Reemplazar VRFR_Tick original por uno que también gestione el timeout
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

    // 2. Recepción de DTMF
    char c = BK4829_ReadDTMFDigit();
    if (c != '\0') {
        if (s_rxState == RX_STATE_DATA && c == 'D') {
            if (s_rxDataIdx > 0) {
                s_rxCrc = s_rxData[s_rxDataIdx - 1];
                s_rxData[s_rxDataIdx - 1] = '\0'; 
            } else {
                s_rxCrc = '0';
            }
            VRFR_ProcessPayload();
            s_rxState = RX_STATE_IDLE;
            
            // Si recibimos cualquier trama válida (que processpayload ya verificó), 
            // asumimos que nos están contestando (o enviando un comando).
            // Si estábamos esperando ACK y llegó, lo manejamos.
            if (strcmp(g_lastRxData, VRFR_CMD_ACK) == 0 || strstr(g_statusMsg, "ACK OK") != NULL) {
                s_txState = TX_STATE_IDLE;
                s_txRetries = 0;
            }
            
        } else {
            VRFR_HandleIncomingTone(c);
        }
    }
}

void VRFR_ProcessLocalKey(uint8_t key) {
    // Ejemplo de llamadas locales desde botones
    if (key == 1) {
        VRFR_SendPayload(VRFR_CMD_LIGHT_OFF, "0");
    } else if (key == 2) {
        VRFR_SendPayload(VRFR_CMD_LIGHT_ON, "0");
    } else if (key == 3) {
        VRFR_SendPayload(VRFR_CMD_CNT, "0");
    }
}

void VRFR_Init(void) {
    s_randomSeed = g_SystemTick + 123;
}
