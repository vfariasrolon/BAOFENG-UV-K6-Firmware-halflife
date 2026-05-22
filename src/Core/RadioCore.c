#include "../Common/includes.h"
#include "RadioCore.h"
#include "TimeManager.h"
#include "../Driver/BK4829_Minimal.h"

typedef enum {
    JUMP_STATE_IDLE,
    JUMP_STATE_WAIT_LOCK,
} JumpState_t;

static JumpState_t s_jumpState = JUMP_STATE_IDLE;
static uint32_t s_jumpStartTime = 0;

void Core_JumpFrequency(uint32_t target_freq) {
    // Escribir registros de frecuencia
    BK4829_WriteReg(0x38, (uint16_t)target_freq);
    BK4829_WriteReg(0x39, (uint16_t)(target_freq >> 16));
    
    // Iniciar FSM no bloqueante
    s_jumpStartTime = g_SystemTick;
    s_jumpState = JUMP_STATE_WAIT_LOCK;
}

void Core_JumpTick(void) {
    if (s_jumpState == JUMP_STATE_WAIT_LOCK) {
        // Leer el bit de PLL Lock del BK4829 (Generalmente en Reg 0x01, asumiendo bit 1 o bit 0)
        // El BK4819 lo tenía en 0x01. Consultamos el bit o esperamos timeout.
        uint16_t status = BK4829_ReadReg(0x01);
        
        bool is_locked = (status & 0x0002) != 0; // Check bit 1 (PLL Lock típico)
        
        // Si el PLL se enganchó antes de 5ms, o si ya pasaron 5ms, liberamos.
        // Esto optimiza el salto enormemente frente a un DelayMs(5) fijo.
        if (is_locked || (g_SystemTick - s_jumpStartTime) >= 5) {
            s_jumpState = JUMP_STATE_IDLE;
        }
    }
}

bool Core_IsJumpComplete(void) {
    return (s_jumpState == JUMP_STATE_IDLE);
}
