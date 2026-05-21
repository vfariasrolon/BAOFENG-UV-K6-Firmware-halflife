#include "Sc5260.h"
#include "KD32f328_gpio.h"
#include "KD32f328_spi.h"
#include "KD32f328_rcc.h"
#include "../Common/prototypes.h"
#include <string.h>

#define LCD_CS_H                GPIOB->BSRR = GPIO_Pin_12
#define LCD_CS_L                GPIOB->BRR  = GPIO_Pin_12

#define LCD_RS_H                GPIOC->BSRR = GPIO_Pin_15
#define LCD_RS_L                GPIOC->BRR  = GPIO_Pin_15

#define LCD_RST_H               GPIOB->BSRR = GPIO_Pin_8
#define LCD_RST_L               GPIOB->BRR  = GPIO_Pin_8

U8 gLcdBuffer[8][128];

static void SC5260_delay(U8 i)
{
    while(i--) {
        __ASM volatile("nop");
        __ASM volatile("nop");
    }
}

static void SC5260_writeCmd(U8 dat)
{
    U16 timeout;
    LCD_CS_L;
    LCD_RS_L;
    SC5260_delay(10);
    
    timeout = 10000;
    while(RESET == SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) && timeout--);
    SPI_SendData8(SPI2, dat);
    timeout = 10000;
    while(SET == SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) && timeout--); 
    
    SC5260_delay(10);
    LCD_CS_H;
    SC5260_delay(10);
}	

static void SC5260_writeDat(U8 dat)
{
    U16 timeout = 10000;
    while(RESET == SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) && timeout--);
    SPI_SendData8(SPI2, dat);
    timeout = 10000;
    while(SET == SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) && timeout--);    
}

static void SC5260_SetStartPosition(U8 Line, U8 column)
{
    SC5260_writeCmd(0xB0 | (Line & 0x0f));
    SC5260_writeCmd(0x10 | ((column >> 4) & 0x0f));
    SC5260_writeCmd(column & 0x0f);
}

static void SC5260_FillScreen(U8 value)
{
    int i, j;
    for(i = 0; i < 8; i++) {
        SC5260_SetStartPosition(i, 0);
        LCD_CS_L;
        LCD_RS_H;
        SC5260_delay(10);
        for(j = 0; j < 132; j++) {
            SC5260_writeDat(value);
        }
    }
    SC5260_delay(10);
    LCD_CS_H;
}

void SC5260_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Configuración de Pines de Control (CS=PB12, RST=PB8, RS=PC15) */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_12;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Secuencia de Reset e Inicialización */
    LCD_RST_L;
    DelayMs(10);
    LCD_RST_H;
    DelayMs(100);
    SC5260_writeCmd(0xE2);
    DelayMs(100);

    SC5260_writeCmd(0x2c);
    DelayMs(10);
    SC5260_writeCmd(0x2e);
    DelayMs(10);
    SC5260_writeCmd(0x2f);
    DelayMs(10);

    SC5260_writeCmd(0x23);
    SC5260_writeCmd(0x81);  
    SC5260_writeCmd(0x26);
    SC5260_writeCmd(0xA2);
    SC5260_writeCmd(0xA1); 
    SC5260_writeCmd(0xC0);       
    SC5260_writeCmd(0x40);     
    SC5260_writeCmd(0xAE);  

    SC5260_FillScreen(0x00);
    memset(gLcdBuffer, 0, sizeof(gLcdBuffer));

    SC5260_writeCmd(0xAF);
}

void LCD_UpdateFullScreen(void)
{
    U8 i, j;
    for(i = 0; i < 8; i++) {
        SC5260_SetStartPosition(i, 4);
        LCD_CS_L;
        LCD_RS_H;
        SC5260_delay(10);
        for(j = 0; j < 128; j++) {
            SC5260_writeDat(gLcdBuffer[i][j]);
        }
    }
    SC5260_delay(10);
    LCD_CS_H;
}

void SC5260_ClearArea(U8 posY, U8 posX, U8 length, U8 wide, U8 fillData)
{
    U8 x, y;
    for(y = posY; y < posY + wide && y < 64; y++) {
        for(x = posX; x < posX + length && x < 128; x++) {
            if (fillData) {
                gLcdBuffer[y / 8][x] |= (1 << (y % 8));
            } else {
                gLcdBuffer[y / 8][x] &= ~(1 << (y % 8));
            }
        }
    }
}

void LCD_DrawPixel(U8 x, U8 y, U8 color)
{
    if (x >= 128 || y >= 64) return;
    if (color) {
        gLcdBuffer[y / 8][x] |= (1 << (y % 8));
    } else {
        gLcdBuffer[y / 8][x] &= ~(1 << (y % 8));
    }
}
