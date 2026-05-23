#include "BK4829_Minimal.h"
#include "includes.h"
#include "KD32f328_gpio.h"
#include "../App/AppEventManager.h"
#include "../Driver/minifont.h"
#include "../Driver/Sc5260.h"
#include "../Driver/watchdog.h"
#include "../Protocol/vrfr_proto.h"
#include "../Core/TimeManager.h"

void uartSendChar(unsigned char ch);

uint32_t g_test_freq = 43305000;

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
        uartSendChar(0xFE);
    uartSendChar(devAddr);
    uartSendChar(devData >> 8);
    uartSendChar(devData & 0xFF);
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
    
        uartSendChar(0xFD);
    uartSendChar(devAddr);
    uartSendChar(devData >> 8);
    uartSendChar(devData & 0xFF);
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
    
    BK4829_ApplyProfile(1);
}

void BK4829_ApplyProfile(uint8_t profile_id) {
    // Siempre Soft Reset antes de cambiar de perfil
    BK4829_WriteReg(0x00, 0x0000);
    DelayMs(10);

    if (profile_id == 0) {
        // PERFIL 0: Nuestro Baremetal Actual
        BK4829_WriteReg(0x37, 0x9F1F);
        BK4829_WriteReg(0x36, 0x0022); // Reloj 26MHz
        
        BK4829_WriteReg(0x10, 0x0318);
        BK4829_WriteReg(0x11, 0x033A);
        BK4829_WriteReg(0x12, 0x03DB);
        BK4829_WriteReg(0x13, 0x03DF);
        BK4829_WriteReg(0x14, 0x0210);
        BK4829_WriteReg(0x49, 0x2AB2);
        BK4829_WriteReg(0x7B, 0x73DC);
        
        BK4829_WriteReg(0x40, 0x3516);
        BK4829_WriteReg(0x1C, 0x07C0);
        BK4829_WriteReg(0x1D, 0xE555);
        BK4829_WriteReg(0x1E, 0x4C58);
        BK4829_WriteReg(0x1F, 0x5454); // rfpll_regvco_vbit=0001
        BK4829_WriteReg(0x3E, 0xA037); // fijo
        BK4829_WriteReg(0x4F, 0x3732); // bandpass noise
        BK4829_WriteReg(0x26, 0x13A0); // mejorar distorsion
        
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
        
        BK4829_WriteReg(0x30, 0x0119); // Reset filters BK4829
        BK4829_WriteReg(0x31, 0x8206); // Control de ganancia automático (AGC)
        BK4829_WriteReg(0x32, 0x0060); // Filtros IF (Frecuencia Intermedia)
        BK4829_WriteReg(0x3B, 0x0A0F); // Configuración de ancho de banda (Wide/Narrow)
        
        BK4829_WriteReg(0x51, 0x0000); // CTCSS OFF
        BK4829_WriteReg(0x52, 0x0000); // DCS OFF
        
        BK4829_WriteReg(0x01, 0x3FF0); // Cristal
        BK4829_WriteReg(0x19, 0x1041);
        BK4829_WriteReg(0x7D, 0xE952);
        BK4829_WriteReg(0x48, 0x2340);
        
        BK4829_WriteReg(0x24, 0x807F | (20 << 7));
        BK4829_WriteReg(0x38, 0xC83D);
        BK4829_WriteReg(0x39, 0x0294);
        BK4829_WriteReg(0x43, 0x3028);
        BK4829_WriteReg(0x47, 0x6040); // AGC Table
    } 
    else if (profile_id == 1) {
        // PERFIL 1: CAPTURA EXACTA SPI (SNOOPER) - GOLDEN SEQUENCE
        BK4829_WriteReg(0x00, 0x0000);
        BK4829_WriteReg(0x37, 0x9F1F);
        BK4829_WriteReg(0x36, 0x0022);
        BK4829_WriteReg(0x10, 0x0318);
        BK4829_WriteReg(0x11, 0x033A);
        BK4829_WriteReg(0x12, 0x03DB);
        BK4829_WriteReg(0x13, 0x03DF);
        BK4829_WriteReg(0x14, 0x0210);
        BK4829_WriteReg(0x49, 0x2AB2);
        BK4829_WriteReg(0x7B, 0x73DC);
        BK4829_WriteReg(0x40, 0x3516);
        BK4829_WriteReg(0x1C, 0x07C0);
        BK4829_WriteReg(0x1D, 0xE555);
        BK4829_WriteReg(0x1E, 0x4C58);
        BK4829_WriteReg(0x1F, 0xC65A);
        BK4829_WriteReg(0x3E, 0x94C6);

        // --- Limpieza de generadores de tonos ---
        // Es crítico que los divisores de frecuencia de TX estén en 0 para habilitar la decodificación RX
        BK4829_WriteReg(0x71, 0x0000);
        BK4829_WriteReg(0x72, 0x0000);

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
        BK4829_WriteReg(0x09, 0x006F);
        BK4829_WriteReg(0x09, 0x106B);
        BK4829_WriteReg(0x09, 0x2067);
        BK4829_WriteReg(0x09, 0x3062);
        BK4829_WriteReg(0x09, 0x4050);
        BK4829_WriteReg(0x09, 0x5047);
        BK4829_WriteReg(0x09, 0x603A);
        BK4829_WriteReg(0x09, 0x702C);
        BK4829_WriteReg(0x09, 0x8041);
        BK4829_WriteReg(0x09, 0x9037);
        BK4829_WriteReg(0x09, 0xA025);
        BK4829_WriteReg(0x09, 0xB017);
        BK4829_WriteReg(0x09, 0xC0E4);
        BK4829_WriteReg(0x09, 0xD0CB);
        BK4829_WriteReg(0x09, 0xE0B5);
        BK4829_WriteReg(0x09, 0xF09F);
        BK4829_WriteReg(0x72, 0x3065);
        BK4829_WriteReg(0x5C, 0x5665);
        BK4829_WriteReg(0x5D, 0x0F00);
        BK4829_WriteReg(0x01, 0x3FF0);
        BK4829_WriteReg(0x19, 0x1041);
        BK4829_WriteReg(0x7D, 0xE952);
        
        // --- AF DAC y Control de Audio ---
        // En la captura de arranque, 0x48 era 0x2340 (MUTE). Cuando se abre el squelch,
        // el firmware original escribe 0x47=0x6042 y 0x48=0x82DF (AF_DAC_EN = 1).
        BK4829_WriteReg(0x47, 0x6142);
        BK4829_WriteReg(0x48, 0xB3FF); // B3FF = DAC Enable (Bit 15=1) + Max Vol
        
        BK4829_WriteReg(0x30, 0xBFF1);
        
        // --- Tuning a 433.050 MHz (Extracción 10:40:40 y 11:02:24) ---
        BK4829_WriteReg(0x30, 0x0000); // IDLE
        BK4829_WriteReg(0x38, 0xC83D); // 0xC83D
        BK4829_WriteReg(0x39, 0x0294); // 0x0294
        BK4829_WriteReg(0x43, 0x3028);
        BK4829_WriteReg(0x78, 0x2040); // Umbral de Squelch
        
        // --- Configuración de Squelch Digital / Filtros ---
        // Restauramos 0x51 a su valor original de fábrica (0x904B). 
        // Ponerlo en 0 apagaba el decodificador de tonos (CTCSS y DTMF comparten ruta).
        BK4829_WriteReg(0x51, 0x904B);
        BK4829_WriteReg(0x07, 0x0810);
        BK4829_WriteReg(0x07, 0x21CD);
        BK4829_WriteReg(0x52, 0x0292);
        
        BK4829_WriteReg(0x31, 0xFFFD); // AGC según captura de las 11:02:24
        BK4829_WriteReg(0x40, 0x34E0);
        
        // --- Habilitar Hardware DTMF Decoder (Faltante en Snooper pasivo) ---
        BK4829_WriteReg(0x24, 0x8A7F); // 0x807F | (20 << 7) = DTMF Threshold + Enable
        BK4829_WriteReg(0x3F, 0x0800); // FSK/DTMF Interrupt Mask Enable
        // La habilitación RX se hace al final de la función
    }
    else if (profile_id == 2) {
        // PERFIL 2: BK4819 Legacy OEM Crudo
        BK4829_WriteReg(0x00, 0x8000); // BK4819 extra reset
        BK4829_WriteReg(0x00, 0x0000);
        BK4829_WriteReg(0x37, 0x1D0F); // REG_37 genérico
        
        BK4829_WriteReg(0x13, 0x03BE);
        BK4829_WriteReg(0x12, 0x037B);
        BK4829_WriteReg(0x11, 0x027B);
        BK4829_WriteReg(0x10, 0x007A);
        BK4829_WriteReg(0x14, 0x0019);
        BK4829_WriteReg(0x49, 0x2A38);
        BK4829_WriteReg(0x7B, 0x8420);
        
        BK4829_WriteReg(0x19, 0x1041);
        BK4829_WriteReg(0x2A, 0x4F18);
        
        BK4829_WriteReg(0x40, 0x3500);
        BK4829_WriteReg(0x1C, 0x0320);
        BK4829_WriteReg(0x1D, 0xE756);
        BK4829_WriteReg(0x1E, 0x4658);
        BK4829_WriteReg(0x1F, 0x444A);
        BK4829_WriteReg(0x3E, 0x98C6);
        
        BK4829_WriteReg(0x73, 0x3691);
        BK4829_WriteReg(0x77, 0x08E2);
        BK4829_WriteReg(0x28, 0x0B40);
        BK4829_WriteReg(0x29, 0xBA00);
        BK4829_WriteReg(0x2C, 0x1812);
        BK4829_WriteReg(0x2F, 0x8080);
        BK4829_WriteReg(0x53, 0x0008);
        BK4829_WriteReg(0x7E, 0x303E);
        BK4829_WriteReg(0x46, 0x600A);
        BK4829_WriteReg(0x4A, 0x5430);
        BK4829_WriteReg(0x07, 0x61CE);
        
        // Coeficientes DTMF Crudos OEM
        BK4829_WriteReg(0x09, 0x006F);
        BK4829_WriteReg(0x09, 0x106B);
        BK4829_WriteReg(0x09, 0x2067);
        BK4829_WriteReg(0x09, 0x3062);
        BK4829_WriteReg(0x09, 0x4050);
        BK4829_WriteReg(0x09, 0x5047);
        BK4829_WriteReg(0x09, 0x603A);
        BK4829_WriteReg(0x09, 0x702C);
        BK4829_WriteReg(0x09, 0x8041);
        BK4829_WriteReg(0x09, 0x9037);
        BK4829_WriteReg(0x09, 0xA025);
        BK4829_WriteReg(0x09, 0xB017);
        BK4829_WriteReg(0x09, 0xC0E4);
        BK4829_WriteReg(0x09, 0xD0CB);
        BK4829_WriteReg(0x09, 0xE0B5);
        BK4829_WriteReg(0x09, 0xF09F);
        
        BK4829_WriteReg(0x72, 1024); // FSK_BAUD
        BK4829_WriteReg(0x5C, 0x5665);
        BK4829_WriteReg(0x5D, (16 * 2 - 1) << 8); // FSK_LEN
        
        BK4829_WriteReg(0x01, 0x3FF0);
        BK4829_WriteReg(0x7D, 0xE95F); // Max MicSens
        
        BK4829_WriteReg(0x48, 0xB3FF); // Max Vol
        
        // Rfic_BandInitial
        BK4829_WriteReg(0x38, 0xC83D);
        BK4829_WriteReg(0x39, 0x0294);
        BK4829_WriteReg(0x43, 0x3028); // BAND_WIDE
        
        BK4829_WriteReg(0x1F, 0x5454);
        BK4829_WriteReg(0x3E, 0xA037);
        BK4829_WriteReg(0x77, 0x88EF);
        BK4829_WriteReg(0x4F, 0x3732);
        BK4829_WriteReg(0x26, 0x13A0);
    }
    
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
        
        // Switch interno BK4829: Ambos en LOW para RX (Driver original no los enciende)
        BK4829_SetGpio(0x0008, 0); // RF_GPIO3 = LOW
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
        BK4829_WriteReg(0x48, 0xB3FF); // DAC enable (0xB000) + Volumen Máximo (0x03FF)
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

// ==========================================
// MÓDEM FSK NATIVO (1200 bps)
// ==========================================

void BK4829_SendFSKData(const uint8_t* pData, uint8_t length) {
    if (length > 64) length = 64; 
    
    // 1. Activar TX y PA
    BK4829_TxEnable(true);
    DelayMs(50); 
    
    // 2. Configurar el módem FSK para TX
    BK4829_WriteReg(0x58, 0x00C1); // FSK Enable, FSK 1.2K
    BK4829_WriteReg(0x72, 0x306A); // Frecuencia exacta 1200Hz OEM
    BK4829_WriteReg(0x70, 0x0040); // IMPORTANTE: TX gain DEBE ser 0x0040 según OEM
    
    // 3. Limpiar FIFO y sincronizar (Valores exactos del OEM)
    BK4829_WriteReg(0x5A, 0x85CF); 
    BK4829_WriteReg(0x5B, 0xAB45); 
    BK4829_WriteReg(0x5C, g_fsk_test_configs[g_fsk_current_cfg].crc_reg); // CRC Dinámico
    
    BK4829_WriteReg(0x59, 0x8028); // Clear TX FIFO (Usando base 0x0028 como OEM)
    BK4829_WriteReg(0x59, 0x0028); // Idle
    
    // 4. Llenar el FIFO FSK
    for (uint8_t i = 0; i < length; i += 2) {
        uint16_t word = pData[i];
        if (i + 1 < length) {
            word |= (pData[i + 1] << 8);
        }
        BK4829_WriteReg(0x5F, word);
    }
    
    // El chip requiere configurar el Length DESPUÉS de llenar el FIFO en TX
    BK4829_WriteReg(0x5D, ((length - 1) << 8)); // FSK Data Length 
    
    // 5. Iniciar transmisión FSK (Bit 11 = 0x0800)
    BK4829_WriteReg(0x59, 0x0828); 
    
    // 6. Esperar a que termine
    uint16_t wait_ms = (length * 10) + 150;
    DelayMs(wait_ms);
    
    // 7. Apagar módem FSK de TX
    BK4829_WriteReg(0x59, 0x0028);
    BK4829_SetAudioMute(true);
    
    // Preparar el módem para escuchar una respuesta
    BK4829_PrepareFSKReceive();
}

void BK4829_PrepareFSKReceive(void) {
    // 1. Apagar interrupciones y limpiar
    BK4829_WriteReg(0x3F, 0x0000);
    BK4829_WriteReg(0x59, 0x0028);
    DelayMs(10);
    
    // 2. Configurar el módem FSK para RX 
    BK4829_WriteReg(0x58, 0x00C1); // FSK Enable, FSK 1.2K
    BK4829_WriteReg(0x72, 0x306A); // Frecuencia exacta 1200Hz OEM
    BK4829_WriteReg(0x70, 0x0000); // IMPORTANTE: RX gain DEBE ser 0x0000 según OEM
    
    uint8_t target_len = g_fsk_test_configs[g_fsk_current_cfg].payload_len;
    BK4829_WriteReg(0x5D, ((target_len - 1) << 8)); // Dinámico: RX espera EXACTAMENTE length bytes
    
    BK4829_WriteReg(0x5A, 0x85CF); 
    BK4829_WriteReg(0x5B, 0xAB45); 
    BK4829_WriteReg(0x5C, g_fsk_test_configs[g_fsk_current_cfg].crc_reg); // CRC Dinámico
    
    // 3. Reactivar RX FSK
    BK4829_RxEnable(true);
    BK4829_WriteReg(0x3F, 0x2000); // Activar Interrupción FSK_RX_FINISHED (Bit 13 en BK4829)
    
    BK4829_WriteReg(0x59, 0x4028); // Limpiar RX FIFO
    BK4829_WriteReg(0x59, 0x1028); // Iniciar FSK RX (Bit 12, SIN Scramble)
}

extern void uartSendChar(unsigned char ch);
void uartSendString(const char* str) {
    while(*str) { uartSendChar(*str++); }
}

uint8_t BK4829_GetFSKData(uint8_t* out_buffer) {
    // DIAGNÓSTICO: Rastrear cambios en 0x0B y 0x0C
    static uint16_t last_reg0b = 0xFFFF;
    static uint16_t last_reg0c = 0xFFFF;
    
    uint16_t reg0b = BK4829_ReadReg(0x0B);
    uint16_t reg0c = BK4829_ReadReg(0x0C);
    
    if (reg0b != last_reg0b || reg0c != last_reg0c) {
        char msg[64];
        snprintf(msg, sizeof(msg), "\r\n[FSK] 0x0B:%04X | 0x0C:%04X\r\n", reg0b, reg0c);
        uartSendString(msg);
        last_reg0b = reg0b;
        last_reg0c = reg0c;
    }
    
    uint8_t target_len = g_fsk_test_configs[g_fsk_current_cfg].payload_len;
    bool use_drain = g_fsk_test_configs[g_fsk_current_cfg].use_drain_mode;
    
    // Ya no usamos use_drain, usaremos una sola lógica infalible
    if ((reg0c & 0x0002) == 0) { // Bit 1 = FSK_RX_SYNC
        return 0; // Nada recibido aún
    }
    
    BK4829_WriteReg(0x02, 0x0000); // Limpiar flags
    
    uint8_t words_read = 0;
    uint8_t words_expected = target_len / 2;
    uint32_t start_time = g_SystemTick;
    
    while (words_read < words_expected) {
        uint16_t word = BK4829_ReadReg(0x5F);
        
        if (word != 0xC400 && word != 0x0000) { 
            // 0xC400 indica FIFO vacío en el BK4819
            out_buffer[words_read * 2] = word & 0xFF;
            if ((words_read * 2 + 1) < target_len) {
                out_buffer[words_read * 2 + 1] = (word >> 8) & 0xFF;
            }
            words_read++;
        } else {
            // El FIFO está vacío temporalmente, esperar un poco (1200bps = ~8.3ms por byte)
            DelayMs(4);
        }
        
        if (g_SystemTick - start_time > 250) {
            break; // Timeout de seguridad si el paquete se cortó a la mitad
        }
    }
    
    BK4829_WriteReg(0x59, 0x4028); // Clear RX FIFO
    BK4829_WriteReg(0x59, 0x1028); // Volver a habilitar RX
    
    return words_read * 2;
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
    // 1. Mostrar UI
    UI_ClearLine(16);
    UI_DrawText(0, 16, "[TEST] FSK TX   ", SCALE_TINY);
    LCD_UpdateFullScreen();
    
    // 2. Iniciar modulación FSK
    uint8_t test_data[] = {'T', 'E', 'S', 'T'};
    BK4829_SendFSKData(test_data, sizeof(test_data));
    
    // 3. Volver a IDLE
    UI_ClearLine(16);
    UI_DrawText(0, 16, "[TEST] IDLE     ", SCALE_TINY);
    LCD_UpdateFullScreen();
}

// ==========================================
// USART SNOOPER
// ==========================================
#include "KD32f328_usart.h"
#include "KD32f328_rcc.h"

void Usart_Init(void) {
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
    
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void uartSendChar(unsigned char ch) {
    USART1->TDR = (ch & (uint16_t)0x01FF);
    uint32_t timeout = 100000;
    while (((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET) && --timeout); 
}
