#ifndef __BK4829_MINIMAL_H
#define __BK4829_MINIMAL_H

#include "PublType.h"

// Inicialización de energía y frecuencia (433.050 MHz)
void BK4829_Init(void);

// Controles de Transmisión y Recepción
void BK4829_TxEnable(bool enable);
void BK4829_RxEnable(bool enable);

// Controles de Audio y DTMF
void BK4829_SetAudioMute(bool mute);
void BK4829_PlayLocalBeep(uint16_t freq_hz, uint16_t duration_ms);
void BK4829_SendDTMF(uint16_t tone1_hz, uint16_t tone2_hz);
void BK4829_StopDTMF(void);

// Lectura de Registros (Diagnóstico)
uint16_t BK4829_ReadReg(uint8_t devAddr);

// Lógica de Protocolo y Test
void BK4829_TestBench_UpdateStatus(bool tx_active);
void BK4829_Test_Carrier5s(void);
void BK4829_Test_DTMF_RF(void);

#endif /* __BK4829_MINIMAL_H */
