#ifndef __MINIFONT_H
#define __MINIFONT_H

#include "PublType.h"

#define SCALE_TINY   1
#define SCALE_NORMAL 2
#define SCALE_LARGE  3

void LCD_DrawChar(U8 x, U8 y, char c, U8 scale);
void UI_DrawText(U8 x, U8 y, const char* str, U8 scale);

#endif /* __MINIFONT_H */
