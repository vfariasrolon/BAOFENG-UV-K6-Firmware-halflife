#include "includes.h"
#include "AppEventManager.h"
#include "../Driver/led.h"
#include "../Common/prototypes.h"
#include "../Driver/Sc5260.h"
#include "../Driver/minifont.h"
#include "../Protocol/vrfr_proto.h"

volatile UI_State_Enum g_uiState = UI_STATE_TEST_BENCH;
volatile U8 g_vrfr_tx_blink_counter = 0;

const char* Debug_GetKeyName(KeyID_Enum key)
{
    switch (key) {
        case KEYID_0: return "0";
        case KEYID_1: return "1";
        case KEYID_2: return "2";
        case KEYID_3: return "3";
        case KEYID_4: return "4";
        case KEYID_5: return "5";
        case KEYID_6: return "6";
        case KEYID_7: return "7";
        case KEYID_8: return "8";
        case KEYID_9: return "9";
        case KEYID_STAR: return "*";
        case KEYID_WELL: return "#";
        case KEYID_MENU: return "MENU";
        case KEYID_UP: return "UP";
        case KEYID_DOWN: return "DOWN";
        case KEYID_EXIT: return "EXIT";
        case KEYID_SIDEKEY1: return "SIDEKEY1";
        case KEYID_SIDEKEY2: return "SIDEKEY2";
        case KEYID_PTT: return "PTT";
        case KEYID_SCAN: return "SCAN";
        case KEYID_AB: return "A/B";
        case KEYID_BAND: return "BAND";
        case KEYID_VM: return "V/M";
        default: return "UNKNOWN";
    }
}

void Protocol_VRFR_Transmit(void)
{
    /* En lugar de bloquear la CPU, delegamos el parpadeo a una tarea en background */
    g_vrfr_tx_blink_counter = 6; // 3 encendidos + 3 apagados (manejado en AppTask.c)
}

void App_EventManager(KeyID_Enum key)
{
    if (key == KEYID_NONE) {
        return;
    }

    switch (g_uiState) {
        case UI_STATE_MAIN:
            if (key == KEYID_SIDEKEY1)
            {
                g_uiState = UI_STATE_VRFR_MENU;
            }
            else if (key == KEYID_SIDEKEY2)
            {
                Light_BacklightToggle();
                Light_LedTopToggle();
            }
            break;

        case UI_STATE_VRFR_MENU:
            /* Botón Lateral Superior (SideKey1) avanza a esperar PTT */
            if (key == KEYID_SIDEKEY1) {
                g_uiState = UI_STATE_VRFR_WAIT_PTT;
            }
            /* Si se presiona EXIT o Menu se podría volver, pero lo dejamos simple por ahora */
            break;

        case UI_STATE_VRFR_WAIT_PTT:
            /* PTT para transmitir, cualquier otra tecla cancela (opcional) */
            if (key == KEYID_PTT) {
                Protocol_VRFR_Transmit();
                g_uiState = UI_STATE_MAIN; /* Volver a main al finalizar */
            } else if (key == KEYID_EXIT || key == KEYID_SIDEKEY2) {
                g_uiState = UI_STATE_MAIN; /* Cancelar */
            }
            break;
            
        case UI_STATE_DEBUG_MAPPING:
            // SC5260_ClearArea toma(posY, posX, length(ancho), wide(alto), color)
            SC5260_ClearArea(20, 0, 128, 30, 0); // Limpiar zona central sin desbordar el buffer
            UI_DrawText(20, 20, Debug_GetKeyName(key), SCALE_NORMAL); // SCALE_NORMAL cabe mejor
            LCD_UpdateFullScreen();
            break;

        case UI_STATE_TEST_BENCH:
        {
            static bool s_first_run = true;
            if (s_first_run) {
                SC5260_ClearArea(0, 0, 128, 64, 0);
                LCD_UpdateFullScreen();
                s_first_run = false;
            }
            
            if (key == KEYID_MENU) {
                UI_ClearLine(0);
                UI_DrawText(0, 0, "[TX] PING", SCALE_TINY);
                UI_ClearLine(8);
                UI_DrawText(0, 8, "DTMF: *10000C   ", SCALE_TINY);
                LCD_UpdatePages(0, 1); // Actualizar solo páginas 0 y 1
                
                VRFR_RenderDiagnostics(); // Pintar logs (que a su vez actualizan su zona)
                
                VRFR_Test_Send_Ping(0);
            } else if (key == KEYID_UP) {
                UI_ClearLine(0);
                UI_DrawText(0, 0, "[TX] HOP+", SCALE_TINY);
                UI_ClearLine(8);
                UI_DrawText(0, 8, "DTMF: *399994   ", SCALE_TINY);
                LCD_UpdatePages(0, 1);
                
                VRFR_RenderDiagnostics();
                
                VRFR_Test_Send_FreqJump(1);
            } else if (key == KEYID_DOWN) {
                UI_ClearLine(0);
                UI_DrawText(0, 0, "[TX] HOP-", SCALE_TINY);
                UI_ClearLine(8);
                UI_DrawText(0, 8, "DTMF: *388884   ", SCALE_TINY);
                LCD_UpdatePages(0, 1);
                
                VRFR_RenderDiagnostics();
                
                VRFR_Test_Send_FreqJump(-1);
            } else if (key == KEYID_PTT) {
                // Pre-calcular la semilla para mostrarla
                uint32_t simulated_seed = g_aniTable.current_seed;
                // El motor XORShift avanza el estado, por lo que pre-calculamos para mostrar
                uint32_t next_seed = simulated_seed;
                next_seed ^= next_seed << 13;
                next_seed ^= next_seed >> 17;
                next_seed ^= next_seed << 5;
                
                char buf[30];
                UI_ClearLine(0);
                UI_DrawText(0, 0, "[TX] RND SEED", SCALE_TINY);
                UI_ClearLine(8);
                sprintf(buf, "DTMF: *4%05X   ", (unsigned int)(next_seed & 0xFFFFF));
                UI_DrawText(0, 8, buf, SCALE_TINY);
                LCD_UpdatePages(0, 1);
                
                VRFR_RenderDiagnostics();
                
                VRFR_Test_Send_Random(); // Envía y aplica la nueva semilla
            } else {
                UI_ClearLine(0);
                UI_DrawText(0, 0, Debug_GetKeyName(key), SCALE_TINY);
                LCD_UpdatePages(0, 1);
                
                VRFR_RenderDiagnostics();
            }
        } // Fin del bloque UI_STATE_TEST_BENCH
        break;

        default:
            g_uiState = UI_STATE_DEBUG_MAPPING;
            break;
    }
}
