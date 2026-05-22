#ifndef RADIOCORE_H
#define RADIOCORE_H

#include <stdint.h>
#include <stdbool.h>

void Core_JumpFrequency(uint32_t target_freq);
void Core_JumpTick(void);
bool Core_IsJumpComplete(void);

#endif // RADIOCORE_H
