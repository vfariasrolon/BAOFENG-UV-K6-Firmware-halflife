#ifndef __VRFR_PROTO_H
#define __VRFR_PROTO_H

#include "PublType.h"

#define VRFR_PREAMBLE_TONE  14  // DTMF '*' (0xE)
#define VRFR_FOOTER_TONE    15  // DTMF '#' (0xF) (Si se necesita como EOF)

#define MAX_PAYLOAD_LEN 16
#define MAX_WHITELIST   10
#define MAX_CALLLOG     20

typedef enum {
    VRFR_CMD_PROVISION = 0,
    VRFR_CMD_CALL      = 1,
    VRFR_CMD_MSG       = 2,
    VRFR_CMD_HOP       = 3
} VRFR_CmdType_Enum;

typedef struct {
    uint8_t preamble;
    uint8_t cmd_type;
    uint8_t payload[MAX_PAYLOAD_LEN];
    uint8_t payload_len;
    uint8_t checksum;
} VRFR_Frame;

typedef struct {
    uint32_t timestamp;
    uint8_t event_type;
    uint32_t source_ani;
} VRFR_EventRecord;

typedef struct {
    uint32_t my_ani;
    uint32_t current_seed;
    uint32_t whitelist[MAX_WHITELIST];
    VRFR_EventRecord call_log[MAX_CALLLOG];
    uint8_t whitelist_count;
    uint8_t call_log_count;
} VRFR_AniTable;

extern VRFR_AniTable g_aniTable;

/* Inicialización del módulo VRFR */
void VRFR_Init(void);

/* Motor Pseudoaleatorio Xorshift32 */
uint32_t VRFR_Xorshift32(uint32_t *state);

/* Gestor EEPROM Simulado */
void VRFR_EEPROM_SaveProvisionData(uint32_t my_ani, uint32_t seed);
void VRFR_EEPROM_LoadProvisionData(uint32_t *my_ani, uint32_t *seed);

/* Parseador de capa física (RX) */
void VRFR_HandleIncomingTone(uint8_t tone);

/* Test Bench y Logging */
void VRFR_Test_Send_Ping(uint32_t target_ani);
void VRFR_Test_Send_FreqJump(int dir);
uint32_t VRFR_Test_Send_Random(void);
void VRFR_LogEvent(const char* msg);
void VRFR_RenderDiagnostics(void);

#endif /* __VRFR_PROTO_H */
