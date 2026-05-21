#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "PublType.h"

typedef enum {
    KEYID_SIDEKEY1=0, KEYID_SIDEKEY2,
    KEYID_VM=2,      KEYID_AB,  KEYID_BAND,  KEYID_NO,
    KEYID_MENU = 6,  KEYID_UP,  KEYID_DOWN,  KEYID_EXIT,
    KEYID_0 = 10,    KEYID_1,   KEYID_2,     KEYID_3,     
    KEYID_4 = 14,    KEYID_5,   KEYID_6,     KEYID_7,  
    KEYID_8 = 18,    KEYID_9,   KEYID_STAR,  KEYID_WELL,
    KEYID_SCAN = 0x1E,
    KEYID_PTT = 0x1F,
    KEYID_NONE = 0xFF
} KeyID_Enum;

#define KEYSTATE_NONE        0
#define KEYSTATE_CLICKED     1
#define KEYSTATE_RELEASE     2

typedef struct {
    U8 keyEvent;
    U8 keyPara;
} STR_KEYSCAN;

extern volatile STR_KEYSCAN g_keyScan;

void Keyboard_Init(void);
KeyID_Enum GetKeyCode(void);
void KEY_ScanTask(void);

#endif /* __KEYBOARD_H */
