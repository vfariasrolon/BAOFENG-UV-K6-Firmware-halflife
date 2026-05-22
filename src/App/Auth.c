#include "../Common/includes.h"
#include "Auth.h"
#include "../Driver/BK4829_Minimal.h"
#include "../Core/AudioEngine.h"
#include <string.h>

bool g_TacticalMode = false;
char g_AuthCode[5] = "159D";

static char s_dtmfBuffer[5] = {0};
static uint8_t s_dtmfIndex = 0;

void Auth_Init(void) {
    // Modo Táctico por defecto al arrancar
    g_TacticalMode = true; 
    BK4829_SetAudioMute(false); // Squelch abierto para poder escuchar los tonos (debug)
}

void Auth_Tick(void) {
    if (!g_TacticalMode) return;

    // Verificar interrupción de decodificación DTMF del BK4829
    uint16_t reg0c = BK4829_ReadReg(0x0C);
    if (reg0c & 0x0001) {
        // Limpiar interrupción
        BK4829_WriteReg(0x02, 0x0000);
        
        // Leer el dígito
        uint16_t val = (BK4829_ReadReg(0x0B) >> 8) & 0x0F;
        char c = '?';
        if (val < 10) c = '0' + val;
        else if (val == 10) c = 'A';
        else if (val == 11) c = 'B';
        else if (val == 12) c = 'C';
        else if (val == 13) c = 'D';
        else if (val == 14) c = '*';
        else if (val == 15) c = '#';
        
        if (c != '?') {
            s_dtmfBuffer[s_dtmfIndex++] = c;
            s_dtmfBuffer[s_dtmfIndex] = '\0';
            
            if (s_dtmfIndex >= 4) {
                if (strcmp(&s_dtmfBuffer[s_dtmfIndex - 4], g_AuthCode) == 0) {
                    // ¡Handshake exitoso!
                    g_TacticalMode = false;
                    BK4829_SetAudioMute(false); // Abrir squelch
                    
                    // Emitir Beep Digital
                    AudioEngine_PlayAuthBeep();
                    
                    // Limpiar
                    s_dtmfIndex = 0;
                    s_dtmfBuffer[0] = '\0';
                }
            }
            
            // Shift para mantener tamaño de ventana de 4 dígitos
            if (s_dtmfIndex >= 4) {
                s_dtmfBuffer[0] = s_dtmfBuffer[1];
                s_dtmfBuffer[1] = s_dtmfBuffer[2];
                s_dtmfBuffer[2] = s_dtmfBuffer[3];
                s_dtmfIndex = 3;
                s_dtmfBuffer[3] = '\0';
            }
        }
    }
}

void Auth_ToggleMode(bool enable) {
    g_TacticalMode = enable;
    if (g_TacticalMode) {
        BK4829_SetAudioMute(true);
        s_dtmfIndex = 0;
    } else {
        BK4829_SetAudioMute(false);
    }
}
