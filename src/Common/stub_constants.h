#ifndef __STUB_CONSTANTS_H
#define __STUB_CONSTANTS_H

/*
 * stub_constants.h — Fase 1: Heartbeat Minimalista
 *
 * Constantes mínimas necesarias para que AppMenu.c, AppTask.c y main.c
 * compilen sin los headers originales de hardware (keyboard.h, Beep.h, etc.).
 *
 * IMPORTANTE: Los valores numéricos son placeholders funcionales.
 * Deben ser reemplazados por los valores reales del proyecto original
 * una vez que los módulos de Driver/ sean recuperados.
 */

/* ------------------------------------------------------------------ */
/*  Tipos base (si PublType.h no está disponible)                      */
/* ------------------------------------------------------------------ */
#ifndef _PUBLTYPE_H
    typedef unsigned char       U8;
    typedef unsigned short      U16;
    typedef unsigned int        U32;
    typedef unsigned char       Boolean;
    typedef char                String;
    #define TRUE    1
    #define FALSE   0
    #ifndef NULL
        #define NULL    ((void*)0)
    #endif
#endif

/* ------------------------------------------------------------------ */
/*  KEYID — Identificadores de teclas                                  */
/* ------------------------------------------------------------------ */
#define KEYID_NONE          0x00
#define KEYID_MENU          0x01
#define KEYID_UP            0x02
#define KEYID_DOWN          0x03
#define KEYID_EXIT          0x04
#define KEYID_0             0x10
#define KEYID_1             0x11
#define KEYID_2             0x12
#define KEYID_3             0x13
#define KEYID_4             0x14
#define KEYID_5             0x15
#define KEYID_6             0x16
#define KEYID_7             0x17
#define KEYID_8             0x18
#define KEYID_9             0x19
#define KEYID_STAR          0x1A
#define KEYID_WELL          0x1B    /* tecla # */
#define KEYID_SIDEKEY1      0x1C
#define KEYID_SIDEKEY2      0x1D
#define KEYID_SCAN          0x1E
#define KEYID_PWRSW         0x1F
#define KEYID_AB            0x20    /* A/B switch — usado en boot OTAP */

/* ------------------------------------------------------------------ */
/*  BEEP — Tipos de pitido                                             */
/* ------------------------------------------------------------------ */
#define BEEP_NULL           0x00
#define BEEP_FASTSW         0x01
#define BEEP_FMUP           0x02
#define BEEP_FMDOWN         0x03
#define BEEP_EXITMENU       0x04
#define BEEP_ERROR          0x05
#define BEEP_FMSW2          0x06

/* ------------------------------------------------------------------ */
/*  MODE — Estados del despachador principal (AppRunTask)              */
/* ------------------------------------------------------------------ */
#define MODE_MAIN           0x00
#define MODE_MENU           0x01
#define MODE_SLAVE_LISTEN   0x02
#define MODE_MASTER_PAIR    0x03
#define MODE_DASHBOARD      0x04
#define MODE_HL_MENU        0x05
#define MODE_DTMF_ANI       0x06
#define MODE_MONI           0x07
#define MODE_SCAN           0x08
#define MODE_DTMF           0x09
#define MODE_PROGRAM        0x0A
#define MODE_FLASH_PROGRAM  0x0B
#define MODE_FM             0x0C
#define MODE_SEARCH         0x0D
#define MODE_SCAN_QT        0x0E
#define MODE_WEATHER        0x0F
#define MODE_STOPWATCH      0x10

/* ------------------------------------------------------------------ */
/*  RF State — Estados del subsistema de radio                         */
/* ------------------------------------------------------------------ */
#define RF_RX               0x00
#define RF_TX               0x01

#define TX_READY            0x00
#define RX_READY            0x00

/* ------------------------------------------------------------------ */
/*  SCAN State                                                          */
/* ------------------------------------------------------------------ */
#define SCAN_IDLE           0x00

/* ------------------------------------------------------------------ */
/*  LCD / Fuentes (usadas en main.c boot OTAP display)                 */
/* ------------------------------------------------------------------ */
#define FONTSIZE_16x16      0x01
#define FONTSIZE_12x12      0x02
#define LCD_DIS_NORMAL      0x00

/* ------------------------------------------------------------------ */
/*  Audio Voices (usadas en BeepPowerOn)                               */
/* ------------------------------------------------------------------ */
#define vo_Welcome          0x01

/* ------------------------------------------------------------------ */
/*  Lenguaje                                                            */
/* ------------------------------------------------------------------ */
#define LANG_EN             0x00
#define LANG_CN             0x01

/* ------------------------------------------------------------------ */
/*  Estructuras globales mínimas (stubs de tipos)                       */
/*  Las reales vienen de RadioCommon.h / PublType.h                    */
/* ------------------------------------------------------------------ */

/* STR_KEYSCAN — estado del scanner de teclado */
typedef struct {
    U8 keyEvent;
    U8 keyPara;
} STR_KEYSCAN;

/* STR_SCAN_INFO — estado del scanner de frecuencia */
typedef struct {
    U8 state;
} STR_SCAN_INFO;

/* STR_RADIO_INFORM — configuración global del radio */
typedef struct {
    U8 language;
    U8 menuExitTime;
    U8 txPower;
    union {
        U8 Byte;
        struct { U8 b0:1; U8 b1:1; U8 b2:2; U8 b3:4; } Bit;
    } OpFlag1;
    U8 remain0[8];  /* parámetros extendidos Half-Life */
} STR_RADIO_INFORM;

/* ------------------------------------------------------------------ */
/*  Variables globales — declaraciones extern                           */
/*  Las definiciones reales deben estar en un .c (sugerido: stubs.c)   */
/* ------------------------------------------------------------------ */
extern volatile U8          g_10msFlag;
extern volatile U8          g_50msFlag;
extern volatile U8          g_100msFlag;
extern volatile U8          g_500msFlag;
extern U8                   g_rfState;
extern U8                   g_rfTxState;
extern U8                   g_rfRxState;
extern STR_SCAN_INFO        g_scanInfo;
extern STR_RADIO_INFORM     g_radioInform;
extern STR_KEYSCAN          g_keyScan;

#endif /* __STUB_CONSTANTS_H */
