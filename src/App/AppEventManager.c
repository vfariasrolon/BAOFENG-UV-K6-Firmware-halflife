#include "includes.h"
#include "AppEventManager.h"
#include "../Driver/led.h"
#include "../Common/prototypes.h"
#include "../Driver/Sc5260.h"
#include "../Driver/keyboard.h"
#include "../Protocol/vrfr_proto.h"
#include "../Driver/BK4829_Minimal.h"
#include <stdio.h>
#include "../Driver/minifont.h"

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
            // Todo el renderizado y manejo de teclas del Test Bench VRFR ahora se realiza en main.c y vrfr_proto.c
            break;

        default:
            g_uiState = UI_STATE_DEBUG_MAPPING;
            break;
    }
}
