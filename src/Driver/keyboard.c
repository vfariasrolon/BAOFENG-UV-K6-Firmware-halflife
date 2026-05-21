#include "PublType.h"
#include "keyboard.h"
#include "KD32f328_gpio.h"
#include "KD32f328_rcc.h"
#include "prototypes.h"

volatile STR_KEYSCAN g_keyScan = {KEYID_NONE, KEYSTATE_NONE};

/* Constantes y Máscaras del teclado original */
static const U16 keyscanTab1[] = {0x0E11, 0x0E10, 0x0E01, 0x0A11, 0x0611, 0x0C11};  /* Salida HIGH */
static const U16 keyscanTab2[] = {0x0000, 0x0001, 0x0010, 0x0400, 0x0800, 0x0200};  /* Salida LOW  */

#define MASK_MATRIX_LINE           0x6003
#define MASK_MATRIX_LINE_1         0x4003
#define MASK_MATRIX_LINE_2         0x2003
#define MASK_MATRIX_LINE_3         0x6001
#define MASK_MATRIX_LINE_4         0x6002
        
static const KeyID_Enum KEYBOARD_TABLE[][4] = {
    {KEYID_NONE, KEYID_SIDEKEY1, KEYID_NONE, KEYID_SIDEKEY2},
    {KEYID_7,    KEYID_8,        KEYID_WELL, KEYID_9},
    {KEYID_4,    KEYID_5,        KEYID_0,    KEYID_6},
    {KEYID_1,    KEYID_2,        KEYID_STAR, KEYID_3},
    {KEYID_MENU, KEYID_UP,       KEYID_EXIT, KEYID_DOWN},
    {KEYID_VM,   KEYID_AB,       KEYID_NONE, KEYID_BAND},
};

void Keyboard_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. Habilitar reloj para GPIOB, GPIOC y GPIOF */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_GPIOF, ENABLE);

    /* 2. Configurar pines de SALIDA (escaneo) en GPIOB: 0, 4, 9, 10, 11 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_4 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 3. Configurar pines de ENTRADA en GPIOB (14) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 4. Configurar pines de ENTRADA en GPIOC (13, 14) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* 5. Configurar pines de ENTRADA en GPIOF (6) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
    GPIO_Init(GPIOF, &GPIO_InitStructure);
}

static U16 KEY_ReadGpioInput(void)
{
    U8 readBit;
    U16 readData = 0x00;

    readData = GPIO_ReadInputData(GPIOC) & 0x6000;
    
    readBit = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14);
    readData |= readBit;

    readBit = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_6);
    readData |= (readBit << 1);

    return readData;
}

KeyID_Enum GetKeyCode(void)
{
    U8 i;

    for (i = 0; i < 6; i++) {
        GPIO_SetBits(GPIOB, keyscanTab1[i]);
        GPIO_ResetBits(GPIOB, keyscanTab2[i]);
        
        DelayUs(5); /* Retardo de estabilización */
        
        switch (KEY_ReadGpioInput() & MASK_MATRIX_LINE) {
            case MASK_MATRIX_LINE_1: return KEYBOARD_TABLE[i][0];
            case MASK_MATRIX_LINE_2: return KEYBOARD_TABLE[i][1];
            case MASK_MATRIX_LINE_3: return KEYBOARD_TABLE[i][2];
            case MASK_MATRIX_LINE_4: return KEYBOARD_TABLE[i][3];
        }
    }
    return KEYID_NONE;
}

/* Debounce muy básico: requiere dos lecturas consistentes */
void KEY_ScanTask(void)
{
    static KeyID_Enum preKey = KEYID_NONE;
    KeyID_Enum key = GetKeyCode();

    if (preKey == key) {
        if (key != KEYID_NONE) {
            g_keyScan.keyEvent = key;
            g_keyScan.keyPara = KEYSTATE_CLICKED;
        } else {
            g_keyScan.keyEvent = KEYID_NONE;
            g_keyScan.keyPara = KEYSTATE_NONE;
        }
    }
    
    preKey = key;
}
