#include "BK4829_Minimal.h"
#include "includes.h"
#include "KD32f328_gpio.h"
#include "../App/AppEventManager.h"
#include "../Driver/minifont.h"
#include "../Driver/Sc5260.h"
#include "../Driver/watchdog.h"

// ==========================================
// MACROS PARA BIT-BANGING DEL SPI DEL BK4829
// ==========================================
static void GpioModeSwitch(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t isOut) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin;
    if (isOut) {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    } else {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    }
    GPIO_Init(GPIOx, &GPIO_InitStructure);
}

#define RFIC_SCN_H     GPIOB->BSRR = GPIO_Pin_3
#define RFIC_SCN_L     GPIOB->BRR  = GPIO_Pin_3
#define RFIC_SCK_H     GPIOB->BSRR = GPIO_Pin_5
#define RFIC_SCK_L     GPIOB->BRR  = GPIO_Pin_5
#define RFIC_SDA_IN    GpioModeSwitch(GPIOB, GPIO_Pin_6, 0)
#define RFIC_SDA_OUT   GpioModeSwitch(GPIOB, GPIO_Pin_6, 1)
#define RFIC_SDA_H     GPIOB->BSRR = GPIO_Pin_6
#define RFIC_SDA_L     GPIOB->BRR  = GPIO_Pin_6
#define RFIC_SDA_VAL   (GPIOB->IDR & GPIO_Pin_6)

static void Rfic_delay(uint32_t delay) {
    DelayUs(delay);
}

static void Rfic_ByteWrite(uint8_t ByteData) {
    uint8_t i;
    uint8_t MaskData = 0x80;
    
    RFIC_SDA_OUT;
    for(i = 0; i < 8; i++) {
        RFIC_SCK_L;
        if(ByteData & MaskData) {
            RFIC_SDA_H; 
        } else {
            RFIC_SDA_L; 
        }
        Rfic_delay(5);
        RFIC_SCK_H;
        Rfic_delay(5);
        MaskData >>= 1;
    }
}

void BK4829_WriteReg(uint8_t devAddr, uint16_t devData) {
    RFIC_SCN_L;
    Rfic_delay(5);
    Rfic_ByteWrite(devAddr);
    Rfic_ByteWrite(devData >> 8);
    Rfic_ByteWrite(devData);
    RFIC_SCN_H;
    Rfic_delay(5);
    RFIC_SCK_L;
}

void BK4829_ResetBus(void) {
    // Reinicializar los pines GPIO del SPI en caso de bloqueo
    GpioModeSwitch(GPIOB, GPIO_Pin_3, 1); // SCN (SPI)
    GpioModeSwitch(GPIOB, GPIO_Pin_5, 1); // SCK (SPI)
    GpioModeSwitch(GPIOB, GPIO_Pin_6, 1); // SDA (SPI)
    RFIC_SCN_H;
    RFIC_SCK_L;
    RFIC_SDA_H;
    Rfic_delay(50); // Pequeña pausa para estabilizar
}

uint16_t BK4829_ReadReg(uint8_t devAddr) {
    uint16_t MaskData;
    uint16_t devData;
    uint8_t retries = 0;
    uint8_t addr_read = devAddr | 0x80; // Bit alto en 1 indica Read

retry:
    MaskData = 0x8000;
    devData = 0;
    
    RFIC_SCN_L;
    Rfic_delay(5);
    Rfic_ByteWrite(addr_read); // Enviar dirección
    
    RFIC_SCK_L;
    Rfic_delay(5);    
    RFIC_SDA_IN; // Cambiar SDA a entrada para leer
    RFIC_SDA_H;  // Pull-up
    Rfic_delay(5);
    
    while(MaskData) {
        RFIC_SCK_H;
        Rfic_delay(5);
        if(RFIC_SDA_VAL) {
            devData |= MaskData;
        }
        RFIC_SCK_L;
        Rfic_delay(5);
        MaskData >>= 1;
    }
    
    RFIC_SCN_H;
    Rfic_delay(5);
    
    // Sanity Check: Si el bus devuelve 0x0000 o 0xFFFF, probablemente está colgado
    if (devData == 0x0000 || devData == 0xFFFF) {
        retries++;
        if (retries <= 3) {
            BK4829_ResetBus();
            goto retry;
        }
    }
    
    return devData;
}

// ==========================================
// INICIALIZACIÓN BÁSICA Y FRECUENCIA
// ==========================================

// Helper para conmutar los pines internos del BK4829 (Registro 0x33)
static void BK4829_SetGpio(uint16_t gpiox, bool val) {
    uint16_t reg33 = BK4829_ReadReg(0x33);
    reg33 &= ~(gpiox << 8); // Habilitar salida (0 en el byte alto)
    if(val) {
        reg33 |= gpiox;     // 1 en el byte bajo
    } else {
        reg33 &= ~gpiox;    // 0 en el byte bajo
    }
    BK4829_WriteReg(0x33, reg33);
}

void BK4829_Init(void) {
    // 0. Configurar pines GPIO del MCU
    GpioModeSwitch(GPIOB, GPIO_Pin_3, 1); // SCN (SPI)
    GpioModeSwitch(GPIOB, GPIO_Pin_5, 1); // SCK (SPI)
    GpioModeSwitch(GPIOB, GPIO_Pin_6, 1); // SDA (SPI)
    GpioModeSwitch(GPIOA, GPIO_Pin_13, 1); // RF switch UHF
    GpioModeSwitch(GPIOA, GPIO_Pin_14, 1); // RF switch VHF
    GpioModeSwitch(GPIOA, GPIO_Pin_3,  1); // LedRxSwitch / bias audio RX
    GpioModeSwitch(GPIOB, GPIO_Pin_2,  1); // SpeakerSwitch (amp bocina)
    
    // Encender bocina y bias de audio desde el arranque
    GPIOB->BSRR = GPIO_Pin_2;  // SpeakerSwitch(ON)
    GPIOA->BSRR = GPIO_Pin_3;  // LedRxSwitch(ON)
    
    // RF Switch UHF RX: A13=HIGH, A14=LOW
    GPIOA->BRR  = GPIO_Pin_14;
    GPIOA->BSRR = GPIO_Pin_13;
    
    // 1. Soft Reset
    BK4829_WriteReg(0x00, 0x0000);
    DelayMs(10);
    
    // 2. Inicialización COMPLETA según DevFD6818.c (Rfic_Init con g_isBK4829)
    // Habilitar LDO/Band Gap/Clock
    BK4829_WriteReg(0x37, 0x9F1F);
    BK4829_WriteReg(0x36, 0x0022);
    
    BK4829_WriteReg(0x36, 0x0022); // Reloj 26MHz
    
    // 3. Configuración de AGC / LNA Gains para BK4829
    BK4829_WriteReg(0x10, 0x0318);
    BK4829_WriteReg(0x11, 0x033A);
    BK4829_WriteReg(0x12, 0x03DB);
    BK4829_WriteReg(0x13, 0x03DF);
    BK4829_WriteReg(0x14, 0x0210);
    BK4829_WriteReg(0x49, 0x2AB2);
    BK4829_WriteReg(0x7B, 0x73DC);
    
    // 4. Audio, PLL, VCO y Modulación (Bypass y Offset)
    BK4829_WriteReg(0x40, 0x3516);
    BK4829_WriteReg(0x1C, 0x07C0);
    BK4829_WriteReg(0x1D, 0xE555);
    BK4829_WriteReg(0x1E, 0x4C58);
    BK4829_WriteReg(0x1F, 0xC65A);
    BK4829_WriteReg(0x3E, 0x94C6);
    
    // 5. Filtros y Preamplificación de Audio
    BK4829_WriteReg(0x73, 0x4691);
    BK4829_WriteReg(0x77, 0x88EF);
    BK4829_WriteReg(0x28, 0x0B40);
    BK4829_WriteReg(0x29, 0xAA00);
    BK4829_WriteReg(0x2A, 0x6600);
    BK4829_WriteReg(0x2C, 0x1822);
    BK4829_WriteReg(0x2F, 0x9890);
    BK4829_WriteReg(0x53, 0x2028);
    BK4829_WriteReg(0x7E, 0x303E);
    BK4829_WriteReg(0x46, 0x600A);
    BK4829_WriteReg(0x4A, 0x5430);
    BK4829_WriteReg(0x07, 0x61CE);
    
    // 6. Configurar Modo de Recepción inicial (Secuencia de reinicio de cadena de audio para BK4829)
    BK4829_WriteReg(0x30, 0x0119); // Reset filters BK4829
    BK4829_WriteReg(0x3B, 0x0A0F); // Re-inicia cadena de audio RF a DAC
    BK4829_WriteReg(0x31, 0x1000);
    
    // Reloj de Referencia / Cristal
    BK4829_WriteReg(0x01, 0x3FF0);
    
    // Micrófono y ganancia
    BK4829_WriteReg(0x19, 0x1041);
    BK4829_WriteReg(0x7D, 0xE952);
    
    // Registro Maestro del Squelch por defecto
    BK4829_WriteReg(0x48, 0x2340);
    
    // Activar decodificador DTMF globalmente (Bit 15 = 1, y Threshold)
    BK4829_WriteReg(0x24, 0x807F | (20 << 7));
    
    // Fijar Frecuencia a 433.050 MHz
    uint32_t calcFreq = 43305000;
    BK4829_WriteReg(0x38, (uint16_t)calcFreq);
    BK4829_WriteReg(0x39, (uint16_t)(calcFreq >> 16));
    
    // Configurar Ancho de Banda IF (Wideband) y AGC
    BK4829_WriteReg(0x43, 0x3028);
    BK4829_WriteReg(0x47, 0x6040); // AGC Table
    
    // Forzar switches físicos e internos a modo RX
    BK4829_RxEnable(true);
    
    // Abrir squelch para escuchar el piso de ruido RF
    BK4829_SetAudioMute(false);
}

// ==========================================
// CONTROLES DE TX / RX
// ==========================================
void BK4829_TxEnable(bool enable) {
    if (enable) {
        // RF Switch externo: modo TX UHF
        // A13=LOW, A14=LOW para TX (igual que PWR_TXON en el original)
        GPIOA->BRR = GPIO_Pin_13;
        GPIOA->BRR = GPIO_Pin_14;
        
        // Switch interno BK4829 vaía registro 0x33:
        // GPIO3=LOW, GPIO2=HIGH para TX UHF (según RF_PowerSet TXON)
        BK4829_SetGpio(0x0008, 0); // RF_GPIO3 = LOW
        BK4829_SetGpio(0x0004, 1); // RF_GPIO2 = HIGH
        
        // Inicializar chip para TX (igual que Rfic_ConfigTxMode)
        BK4829_WriteReg(0x30, 0x0000); // IDLE primero
        DelayMs(2);
        
        // PA Control via registro 0x36 (igual que Rfic_SetPA)
        // dat=1 → temp1 = 0x0100, OR con 0xFF = 0x01FF
        BK4829_WriteReg(0x36, 0x01FF);
        
        // Activar TX (valor REAL para BK4829)
        BK4829_WriteReg(0x30, 0xC1FE); // MODO TX ACTIVO NATIVO (0xC1FE)
    } else {
        BK4829_WriteReg(0x30, 0x0000); // IDLE
    }
}

void BK4829_RxEnable(bool enable) {
    if (enable) {
        BK4829_WriteReg(0x02, 0x0000); // Clear Int
        BK4829_WriteReg(0x30, 0x0000); // IDLE primero
        
        // RF Switch externo: modo RX UHF (A13=HIGH, A14=LOW)
        GPIOA->BRR  = GPIO_Pin_14;
        GPIOA->BSRR = GPIO_Pin_13;
        
        // Switch interno BK4829: GPIO3=HIGH, GPIO2=LOW para RX
        BK4829_SetGpio(0x0008, 1); // RF_GPIO3 = HIGH
        BK4829_SetGpio(0x0004, 0); // RF_GPIO2 = LOW
        
        BK4829_WriteReg(0x30, 0xBFF1); // MODO RX ACTIVO NATIVO (0xBFF1)
    } else {
        BK4829_WriteReg(0x30, 0x0000); // IDLE
    }
}

// ==========================================
// AUDIO Y DTMF
// ==========================================
void BK4829_SetAudioMute(bool mute) {
    if (mute) {
        // Cerrar squelch del BK4829 y apagar amplificador externo
        BK4829_WriteReg(0x78, 0x3040); // Squelch Level 8 cerrado
        BK4829_WriteReg(0x48, 0x2340); // Mute maestro
        GPIOB->BRR  = GPIO_Pin_2; // SpeakerSwitch(OFF)
    } else {
        // Abrir squelch físico y encender amplificador de la bocina
        BK4829_WriteReg(0x78, 0x0000); // Squelch ABIERTO (Piso de ruido libre)
        BK4829_WriteReg(0x48, 0x8192); // DAC enable (0x8000) + Volumen + Gain
        GPIOB->BSRR = GPIO_Pin_2; // SpeakerSwitch(ON)
    }
}

void BK4829_ForceOpenAudio(void) {
    // 1. Amplificador físico ON
    GPIOB->BSRR = GPIO_Pin_2;
    // 2. Bias de audio ON
    GPIOA->BSRR = GPIO_Pin_3;
    // 3. Forzar DAC a volumen máximo y quitar mute
    BK4829_WriteReg(0x78, 0x0000); // FORZAR SQUELCH ABIERTO
    BK4829_WriteReg(0x48, 0x83F2); // 0x8000 (ON) | 0x03F0 (Vol Max) | 0x0002 (Gain)
    // 4. Enrutamiento AFOUT RX hacia DAC interno
    BK4829_WriteReg(0x47, 0x6142); // 0x6042 nativo + 0x0100 (RX AFOUT)
    // 5. Configurar filtros base
    BK4829_WriteReg(0x70, 0x00E0);
    BK4829_WriteReg(0x74, 0x3B2D);
    // 6. Reset de cadena de audio y enganchar RXON con todos los LDOs y Clock (0xBFF1)
    BK4829_WriteReg(0x30, 0x0119);
    DelayMs(10);
    BK4829_WriteReg(0x30, 0xBFF1); // ¡EL VALOR CORRECTO DEL DRIVER ORIGINAL (LDOs + Clock + RX)!
}

// Macro que convierte Hz al formato del registro BK4829
#define BK4829_HZ_TO_REG(hz)  ((uint16_t)((uint32_t)(hz) * 1032444UL / 100000UL))

void BK4829_PlayLocalBeep(uint16_t freq_hz, uint16_t duration_ms) {
    // 1. Encender bocina si no está encendida y abrir Squelch
    BK4829_SetAudioMute(false);
    
    // 2. Modo TONE local (0x0302)
    BK4829_WriteReg(0x30, 0x0302);
    
    // 3. Gain para el tono (bit 15 = enable, bits 14-8 = gain)
    BK4829_WriteReg(0x70, 0x00E0 | 0x8000 | (60 << 8));
    
    // 4. Programar frecuencia convertida al formato BK4829
    BK4829_WriteReg(0x71, BK4829_HZ_TO_REG(freq_hz));
    
    // 5. Habilitar salida de tono
    BK4829_WriteReg(0x3F, 0x0800);
    
    // 6. Esperar duración
    DelayMs(duration_ms);
    
    // 7. Apagar tono y volver a RX NATIVO
    BK4829_WriteReg(0x3F, 0x0000);
    BK4829_WriteReg(0x70, 0x00E0); // Restaurar gain LNA
    BK4829_WriteReg(0x30, 0xBFF1); // Volver a RX NATIVO
    BK4829_WriteReg(0x48, 0x2340); // Cerrar squelch
}

void BK4829_PlayLocalDTMF(uint16_t tone1_hz, uint16_t tone2_hz, uint16_t duration_ms) {
    // Tono DTMF LOCAL: suena en la bocina sin transmitir RF (Modo TONE = 0x0002)
    
    // 1. Encender bocina y abrir squelch de audio
    GPIOB->BSRR = GPIO_Pin_2;
    BK4829_WriteReg(0x48, 0x0000); 
    
    // 2. Modo TONE local (NO RF, valor real BK4829)
    BK4829_WriteReg(0x30, 0x0302);
    
    // 3. Threshold DTMF y Ganancia para ambos tonos
    BK4829_WriteReg(0x24, 0x807F | (20 << 7));
    BK4829_WriteReg(0x70, 0xE0E0); // Ganancia máxima en tono1 y tono2
    
    // 4. Programar frecuencias
    BK4829_WriteReg(0x71, BK4829_HZ_TO_REG(tone1_hz));
    BK4829_WriteReg(0x72, BK4829_HZ_TO_REG(tone2_hz));
    
    // 5. Habilitar salida
    BK4829_WriteReg(0x3F, 0x0800);
    
    // 6. Esperar duración
    DelayMs(duration_ms);
    
    // 7. Apagar todo
    BK4829_WriteReg(0x3F, 0x0000);
    uint16_t reg24 = BK4829_ReadReg(0x24) & 0xFFDF;
    BK4829_WriteReg(0x24, reg24);
    BK4829_WriteReg(0x70, 0x00E0); // Restaurar gain LNA
    BK4829_WriteReg(0x30, 0xBFF1); // Volver a RX (BK4829)
}

void BK4829_PlayDTMFString(const char* digits) {
    // Reproduce una cadena de texto como tonos DTMF (ej. "1234")
    // Útil para ringtones ANI o beeps de arranque estilo celular
    while (*digits) {
        uint16_t t1 = 0, t2 = 0;
        char c = *digits;
        
        if (c == '1') { t1 = 697; t2 = 1209; }
        else if (c == '2') { t1 = 697; t2 = 1336; }
        else if (c == '3') { t1 = 697; t2 = 1477; }
        else if (c == 'A') { t1 = 697; t2 = 1633; }
        else if (c == '4') { t1 = 770; t2 = 1209; }
        else if (c == '5') { t1 = 770; t2 = 1336; }
        else if (c == '6') { t1 = 770; t2 = 1477; }
        else if (c == 'B') { t1 = 770; t2 = 1633; }
        else if (c == '7') { t1 = 852; t2 = 1209; }
        else if (c == '8') { t1 = 852; t2 = 1336; }
        else if (c == '9') { t1 = 852; t2 = 1477; }
        else if (c == 'C') { t1 = 852; t2 = 1633; }
        else if (c == '*') { t1 = 941; t2 = 1209; }
        else if (c == '0') { t1 = 941; t2 = 1336; }
        else if (c == '#') { t1 = 941; t2 = 1477; }
        else if (c == 'D') { t1 = 941; t2 = 1633; }
        
        if (t1 != 0 && t2 != 0) {
            BK4829_PlayLocalDTMF(t1, t2, 100); // 100ms de tono
            DelayMs(50); // 50ms de pausa entre tonos
        }
        digits++;
    }
    
    // Silenciar radio al terminar toda la secuencia
    BK4829_SetAudioMute(true);
}

void BK4829_SendDTMF(uint16_t tone1_hz, uint16_t tone2_hz) {
    // Secuencia exacta de Rfic_EnterDTMFMode + Rfic_SetDtmfFreq del src_sucio
    // Las frecuencias DEBEN convertirse al formato del registro BK4829
    
    // 1. Modo TXTONE (portadora TX + modulación de tono activa, BK4829 nativo)
    BK4829_WriteReg(0x30, 0x0003);
    
    // 2. Threshold DTMF
    BK4829_WriteReg(0x24, 0x807F | (20 << 7));
    
    // 3. Gain máximo para ambos tonos
    BK4829_WriteReg(0x70, 0xE0E0);
    
    // 4. Frecuencias convertidas al formato BK4829 (NO Hz crudos)
    BK4829_WriteReg(0x71, BK4829_HZ_TO_REG(tone1_hz));
    BK4829_WriteReg(0x72, BK4829_HZ_TO_REG(tone2_hz));
    
    // 5. Habilitar salida DTMF interna
    BK4829_WriteReg(0x3F, 0x0800);
}

void BK4829_StopDTMF(bool returnToRx) {
    // 1. Deshabilitar salida DTMF
    BK4829_WriteReg(0x3F, 0x0000);
    
    // 2. Limpiar threshold
    uint16_t reg24 = BK4829_ReadReg(0x24) & 0xFFDF;
    BK4829_WriteReg(0x24, reg24);
    
    // 3. Restaurar Gain LNA/IF
    BK4829_WriteReg(0x70, 0x00E0);
    
    // 4. CRITICO: Volver a modo RX solo si se indica
    if (returnToRx) {
        BK4829_RxEnable(true);
    }
}

void BK4829_SendDTMFStringRF(const char* digits) {
    // Habilitar transmisión física (PA y RF Switch)
    BK4829_TxEnable(true);
    DelayMs(50); // Esperar estabilización del PA
    
    while (*digits) {
        uint16_t t1 = 0, t2 = 0;
        char c = *digits;
        
        if (c == '1') { t1 = 697; t2 = 1209; }
        else if (c == '2') { t1 = 697; t2 = 1336; }
        else if (c == '3') { t1 = 697; t2 = 1477; }
        else if (c == 'A') { t1 = 697; t2 = 1633; }
        else if (c == '4') { t1 = 770; t2 = 1209; }
        else if (c == '5') { t1 = 770; t2 = 1336; }
        else if (c == '6') { t1 = 770; t2 = 1477; }
        else if (c == 'B') { t1 = 770; t2 = 1633; }
        else if (c == '7') { t1 = 852; t2 = 1209; }
        else if (c == '8') { t1 = 852; t2 = 1336; }
        else if (c == '9') { t1 = 852; t2 = 1477; }
        else if (c == 'C') { t1 = 852; t2 = 1633; }
        else if (c == '*') { t1 = 941; t2 = 1209; }
        else if (c == '0') { t1 = 941; t2 = 1336; }
        else if (c == '#') { t1 = 941; t2 = 1477; }
        else if (c == 'D') { t1 = 941; t2 = 1633; }
        
        if (t1 != 0 && t2 != 0) {
            BK4829_SendDTMF(t1, t2); // Inicia modulación RF
            DelayMs(80); // 80ms de tono
            BK4829_StopDTMF(false); // Detener tono pero MANTENER TX
            DelayMs(80); // 80ms de pausa de portadora muda
        }
        digits++;
    }
    
    // Al terminar, deshabilitar transmisión y volver a RX
    BK4829_TxEnable(false);
}

char BK4829_ReadDTMFDigit(void) {
    uint16_t reg0c = BK4829_ReadReg(0x0C);
    if (reg0c & 0x0001) { // DTMF decodificado
        // Limpiar interrupción
        BK4829_WriteReg(0x02, 0x0000);
        
        uint16_t val = (BK4829_ReadReg(0x0B) >> 8) & 0x0F;
        char c = '\0';
        if (val < 10) c = '0' + val;
        else if (val == 10) c = 'A';
        else if (val == 11) c = 'B';
        else if (val == 12) c = 'C';
        else if (val == 13) c = 'D';
        else if (val == 14) c = '*';
        else if (val == 15) c = '#';
        
        return c;
    }
    return '\0';
}

// ==========================================
// TEST BENCH STATUS (Antiguo STUB)
// ==========================================
void BK4829_TestBench_UpdateStatus(bool tx_active) {
    if (g_uiState == UI_STATE_TEST_BENCH) {
        UI_ClearLine(16);
        if (tx_active) {
            UI_DrawText(0, 16, "[TX/RX] TX ON   ", SCALE_TINY);
        } else {
            UI_DrawText(0, 16, "[TX/RX] RX IDLE ", SCALE_TINY);
        }
        LCD_UpdatePages(2, 2);
    }
}

void BK4829_Test_Carrier5s(void) {
    BK4829_TxEnable(true);
    BK4829_TestBench_UpdateStatus(true);
    BK4829_SetAudioMute(false);
    
    // Dejar la portadora encendida por 5 segundos
    for(int i = 0; i < 50; i++) {
        DelayMs(100);
        WDT_Refresh(); // Evitar reinicios del Watchdog
    }
    
    BK4829_SetAudioMute(true);
    BK4829_TxEnable(false);
    BK4829_TestBench_UpdateStatus(false);
}

void BK4829_Test_DTMF_RF(void) {
    // 1. Abrir transmisión física (PA)
    BK4829_TxEnable(true);
    BK4829_TestBench_UpdateStatus(true);
    
    // 2. Iniciar modulación DTMF (usamos 941 y 1336 que son detectables fácilmente)
    BK4829_SendDTMF(941, 1336);
    
    // 3. Transmitir el tono durante 3 segundos
    for(int i = 0; i < 30; i++) {
        DelayMs(100);
        WDT_Refresh();
    }
    
    // 4. Detener modulación
    BK4829_StopDTMF(true);
    
    // 5. Apagar transmisión (PA) y volver a RX
    BK4829_TxEnable(false);
    BK4829_TestBench_UpdateStatus(false);
}
