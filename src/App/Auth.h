#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stdbool.h>

extern bool g_TacticalMode;
extern char g_AuthCode[5];

void Auth_Init(void);
void Auth_Tick(void);
void Auth_ToggleMode(bool enable);

#endif // AUTH_H
