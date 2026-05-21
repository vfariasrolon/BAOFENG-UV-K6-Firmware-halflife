#ifndef __APPEVENTMANAGER_H
#define __APPEVENTMANAGER_H

#include "PublType.h"
#include "../Driver/keyboard.h"

typedef enum {
    UI_STATE_MAIN = 0,
    UI_STATE_VRFR_MENU,
    UI_STATE_VRFR_WAIT_PTT,
    UI_STATE_PROVISIONING,
    UI_STATE_TEST_BENCH,
    UI_STATE_DEBUG_MAPPING,
} UI_State_Enum;

extern volatile UI_State_Enum g_uiState;

void App_EventManager(KeyID_Enum key);
void Protocol_VRFR_Transmit(void);

#endif /* __APPEVENTMANAGER_H */
