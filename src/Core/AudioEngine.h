#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

void AudioEngine_Init(void);

// Reproduce la simulación del Chirp de Nextel
void AudioEngine_PlayNextelChirp(void);

// Reproduce el Beep de Autorización Exitosa
void AudioEngine_PlayAuthBeep(void);

// Reproduce Beep corto para ACK de VRFR
void AudioEngine_PlayAck(void);

// Tarea a llamar desde el scheduler
void AudioEngine_Task(void);

#endif // AUDIO_ENGINE_H
