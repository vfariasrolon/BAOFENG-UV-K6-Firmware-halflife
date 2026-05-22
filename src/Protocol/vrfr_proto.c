#include "vrfr_proto.h"
#include "../App/AppEventManager.h" // Para g_uiState y UI_STATE_PROVISIONING
#include "../Driver/Sc5260.h"
#include "../Driver/minifont.h"
#include "../Driver/BK4829_Minimal.h"
#include <stdio.h>
#include <string.h>

/* Buffer Circular de Logs de Diagnóstico */
static char s_rx_logs[3][17]; // 3 líneas de máx 16 chars + null

/* Estructura central en memoria RAM (simulando memoria persistente) */
VRFR_AniTable g_aniTable;

/* Variables de la Máquina de Estados del Decodificador DTMF */
typedef enum {
    RX_STATE_IDLE,
    RX_STATE_HEADER,
    RX_STATE_PAYLOAD,
    RX_STATE_CHECKSUM
} VRFR_RxState_Enum;

static VRFR_RxState_Enum s_rxState = RX_STATE_IDLE;
static VRFR_Frame s_rxFrame;
static uint8_t s_payloadIndex = 0;

/* Motor Pseudoaleatorio Xorshift32 */
uint32_t VRFR_Xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    /* Prevenir que el estado sea cero (condición fatal para Xorshift) */
    if (x == 0) x = 0xDEADBEEF; 
    
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    
    *state = x;
    return x;
}

/* Stubs de EEPROM */
void VRFR_EEPROM_SaveProvisionData(uint32_t my_ani, uint32_t seed)
{
    g_aniTable.my_ani = my_ani;
    g_aniTable.current_seed = seed;
    // En el futuro: I2C_WriteEEPROM(addr, buffer, size);
}

void VRFR_EEPROM_LoadProvisionData(uint32_t *my_ani, uint32_t *seed)
{
    *my_ani = g_aniTable.my_ani;
    *seed = g_aniTable.current_seed;
}

/* Manejador de Payload tras validación */
static void VRFR_ProcessValidFrame(void)
{
    if (s_rxFrame.cmd_type == VRFR_CMD_PROVISION) {
        /* Para PROVISION, requerimos estar explícitamente en el estado adecuado 
         * y que el payload tenga 12 dígitos: 4 de ANI + 8 de Semilla */
        if (g_uiState == UI_STATE_PROVISIONING && s_rxFrame.payload_len == 12) {
            uint32_t parsed_ani = 0;
            uint32_t parsed_seed = 0;
            uint8_t i;
            
            // Reconstruir ANI (4 nibbles)
            for (i = 0; i < 4; i++) {
                parsed_ani = (parsed_ani << 4) | (s_rxFrame.payload[i] & 0x0F);
            }
            
            // Reconstruir Seed (8 nibbles)
            for (i = 4; i < 12; i++) {
                parsed_seed = (parsed_seed << 4) | (s_rxFrame.payload[i] & 0x0F);
            }
            
            VRFR_EEPROM_SaveProvisionData(parsed_ani, parsed_seed);
            
            VRFR_LogEvent("RX: PROV OK");
            
            // Éxito: Podemos volver a MAIN
            g_uiState = UI_STATE_MAIN;
        } else {
            VRFR_LogEvent("RX: PROV REJECT");
        }
    } else {
        char buf[17];
        snprintf(buf, sizeof(buf), "RX: CMD %d", s_rxFrame.cmd_type);
        VRFR_LogEvent(buf);
    }
}

/* Máquina de estados de capa física (RX Engine) */
void VRFR_HandleIncomingTone(uint8_t tone)
{
    uint8_t expected_payload_len = 0;

    /* Un preámbulo siempre fuerza un reseteo de la máquina de estados */
    if (tone == VRFR_PREAMBLE_TONE) {
        s_rxState = RX_STATE_HEADER;
        s_rxFrame.preamble = tone;
        s_payloadIndex = 0;
        return;
    }

    switch (s_rxState) {
        case RX_STATE_IDLE:
            // Ignorar basura en el aire
            break;

        case RX_STATE_HEADER:
            s_rxFrame.cmd_type = tone;
            s_rxState = RX_STATE_PAYLOAD;
            break;

        case RX_STATE_PAYLOAD:
            s_rxFrame.payload[s_payloadIndex++] = tone;
            
            // Determinar longitud del payload según comando
            if (s_rxFrame.cmd_type == VRFR_CMD_PROVISION) expected_payload_len = 12;
            else if (s_rxFrame.cmd_type == VRFR_CMD_CALL) expected_payload_len = 8; // Ej: 4 src, 4 dst
            else expected_payload_len = 8; // Default genérico

            if (s_payloadIndex >= expected_payload_len) {
                s_rxFrame.payload_len = expected_payload_len;
                s_rxState = RX_STATE_CHECKSUM;
            } else if (s_payloadIndex >= MAX_PAYLOAD_LEN) {
                // Desbordamiento (Corrupción masiva o ataque)
                s_rxState = RX_STATE_IDLE;
            }
            break;

        case RX_STATE_CHECKSUM:
            s_rxFrame.checksum = tone;
            
            // Validar Checksum
            {
                uint8_t calc_sum = s_rxFrame.cmd_type;
                for (uint8_t i = 0; i < s_rxFrame.payload_len; i++) {
                    calc_sum += s_rxFrame.payload[i];
                }
                calc_sum = calc_sum % 16;
                
                if (calc_sum == s_rxFrame.checksum) {
                    VRFR_ProcessValidFrame();
                } else {
                    VRFR_LogEvent("RX: CHK FAIL");
                }
            }
            
            // Volver a estado de reposo, independientemente de si el check falló
            s_rxState = RX_STATE_IDLE;
            break;
    }
}

/* Test Bench y Logging Engine */
void VRFR_LogEvent(const char* msg)
{
    // En lugar de hacer scroll (lo cual confunde visualmente como 'empalme'), 
    // simplemente sobreescribimos el último evento en la línea central.
    strncpy(s_rx_logs[0], msg, 15);
    s_rx_logs[0][15] = '\0';
    
    if (g_uiState == UI_STATE_TEST_BENCH) {
        VRFR_RenderDiagnostics();
        LCD_UpdatePages(4, 6);
    }
}

void VRFR_RenderDiagnostics(void)
{
    // Limpiar toda la zona de logs para evitar basura visual
    UI_ClearLine(32); // Página 4
    UI_ClearLine(40); // Página 5
    UI_ClearLine(48); // Página 6

    // Dibujar el único evento en la página 6 (abajo) con texto estático
    UI_DrawText(0, 48, s_rx_logs[0], SCALE_TINY);
    
    // Solo actualizar las páginas exclusivas de logs (Páginas 4 a 6)
    LCD_UpdatePages(4, 6);
}

extern volatile uint8_t g_vrfr_tx_blink_counter;
extern void DelayMs(uint32_t ms);

void VRFR_Test_Send_Ping(uint32_t target_ani)
{
    // 1. Inicializar hardware y frecuencia base
    BK4829_Init();
    
    // 2. Encender Transmisor y Amplificador
    BK4829_TxEnable(true);
    BK4829_TestBench_UpdateStatus(true);
    
    // 3. Emitir PING DTMF (Tonos simulados: 697 Hz y 1209 Hz -> '1')
    BK4829_SetAudioMute(false); // <--- Habilitar bocina
    BK4829_SendDTMF(697, 1209);
    DelayMs(300); // Duración del tono
    BK4829_StopDTMF();
    BK4829_SetAudioMute(true);  // <--- Mutear bocina
    
    // 4. Apagar Transmisor y volver a RX
    BK4829_TxEnable(false);
    BK4829_TestBench_UpdateStatus(false);
    
    // Feedback local en logs
    VRFR_LogEvent("TX: PING SENT");
}

void VRFR_Test_Send_FreqJump(int dir)
{
    // Igual que PING pero distinto tono o log
    BK4829_TxEnable(true);
    BK4829_TestBench_UpdateStatus(true);
    
    // Tonos: 770 Hz y 1336 Hz -> '5'
    BK4829_SetAudioMute(false);
    BK4829_SendDTMF(770, 1336);
    DelayMs(300);
    BK4829_StopDTMF();
    BK4829_SetAudioMute(true);
    
    BK4829_TxEnable(false);
    BK4829_TestBench_UpdateStatus(false);
    
    VRFR_LogEvent("TX: JUMP SENT");
}

uint32_t VRFR_Test_Send_Random(void)
{
    BK4829_TxEnable(true);
    BK4829_TestBench_UpdateStatus(true);
    
    // Generar un salto random real basado en la semilla local
    uint32_t new_seed = VRFR_Xorshift32(&g_aniTable.current_seed);
    
    // Tonos: 852 Hz y 1477 Hz -> '9'
    BK4829_SetAudioMute(false);
    BK4829_SendDTMF(852, 1477);
    DelayMs(300);
    BK4829_StopDTMF();
    BK4829_SetAudioMute(true);
    
    BK4829_TxEnable(false);
    BK4829_TestBench_UpdateStatus(false);
    
    VRFR_LogEvent("TX: RAND SENT");
    
    return new_seed;
}
