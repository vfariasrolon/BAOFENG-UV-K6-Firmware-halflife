#ifndef __VRFR_PROTO_H
#define __VRFR_PROTO_H

#include "PublType.h"

// Comandos
#define VRFR_CMD_HANDSHAKE   "01"
#define VRFR_CMD_LIGHT_OFF   "07"
#define VRFR_CMD_LIGHT_ON    "08"
#define VRFR_CMD_CNT         "09"
#define VRFR_CMD_ACK         "99"

// Configuración Dinámica FSK
typedef struct {
    uint16_t crc_reg;      // 0xAA30 o 0x5665
    uint8_t payload_len;   // 12 o 16
    bool use_drain_mode;   // false = Bit0, true = Drenar
} FSK_TestConfig_t;

extern uint8_t g_fsk_current_cfg;
extern const FSK_TestConfig_t g_fsk_test_configs[8];

extern uint32_t g_remoteCounter;
extern char g_lastRxData[32];
extern char g_lastTxData[32];

// FSM y Tareas
void VRFR_Init(void);
void VRFR_Tick(void); // Tarea periódica principal (cada 10ms)
void VRFR_HandleIncomingTone(char tone);

// Acciones de Envío
void VRFR_SendPayload(const char* cmd, const char* data);
void VRFR_ProcessLocalKey(uint8_t key); // Invocada desde main.c al pulsar teclas

// Diagnósticos
void VRFR_RenderDiagnostics(void);

// Motor Aleatorio
uint32_t VRFR_Xorshift32(uint32_t *state);

#endif /* __VRFR_PROTO_H */
