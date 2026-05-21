#include "BK4829_stub.h"
#include "../App/AppEventManager.h"
#include "../Protocol/vrfr_proto.h"
#include "../Driver/minifont.h"
#include "../Driver/Sc5260.h"

void BK4829_SetAudioMute(bool mute)
{
    // Silencio táctico: no activamos audio durante Aprovisionamiento
    if (g_uiState == UI_STATE_PROVISIONING) {
        // No logueamos al historial para no hacer ruido visual
        return;
    }

    if (g_uiState == UI_STATE_TEST_BENCH) {
        // Limpiar físicamente la línea usando Dirty Region Renderer
        UI_ClearLine(16);
        
        // En vez de empujar un log circular, actualizamos un indicador fijo en Y=16
        if (mute) {
            UI_DrawText(0, 16, "AUDIO: OFF      ", SCALE_TINY);
        } else {
            UI_DrawText(0, 16, "AUDIO: ON       ", SCALE_TINY);
        }
        
        // La línea Y=16 afecta únicamente a la página 2.
        LCD_UpdatePages(2, 2);
    }
}

void BK4829_SetAudioGain(uint8_t gain)
{
    // Stub temporal
    (void)gain;
}
