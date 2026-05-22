#ifndef __LIGHT_SYSTEM_H
#define __LIGHT_SYSTEM_H

#include "PublType.h"

void LightSystem_Init(void);
void Light_BacklightToggle(void);
void Light_LedTopToggle(void);
void LightSystem_Set(uint8_t on);

#endif /* __LIGHT_SYSTEM_H */
