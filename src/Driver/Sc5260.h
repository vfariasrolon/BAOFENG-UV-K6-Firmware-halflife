#ifndef __SC5260_H
#define __SC5260_H

#include "PublType.h"

extern U8 gLcdBuffer[8][128];

void SC5260_Init(void);
void LCD_UpdateFullScreen(void);
void SC5260_ClearArea(U8 posY, U8 posX, U8 length, U8 wide, U8 fillData);
void LCD_DrawPixel(U8 x, U8 y, U8 color);

#endif /* __SC5260_H */
