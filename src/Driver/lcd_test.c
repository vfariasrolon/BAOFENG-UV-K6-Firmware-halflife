#include "lcd_test.h"
#include "Sc5260.h"
#include "minifont.h"
#include "icons.h"

void LCD_RunDiagnosticTest(void)
{
    U8 x, y;
    
    /* 1. Limpiar pantalla */
    SC5260_ClearArea(0, 0, 128, 64, 0);

    /* 2. Marco Perimetral */
    for (x = 0; x < 128; x++) {
        LCD_DrawPixel(x, 0, 1);
        LCD_DrawPixel(x, 63, 1);
    }
    for (y = 0; y < 64; y++) {
        LCD_DrawPixel(0, y, 1);
        LCD_DrawPixel(127, y, 1);
    }

    /* 3. Cruz Central */
    for (x = 0; x < 128; x++) {
        LCD_DrawPixel(x, 32, 1);
    }
    for (y = 0; y < 64; y++) {
        LCD_DrawPixel(64, y, 1);
    }

    /* 4. Vértices Adicionales (puntos gruesos) */
    LCD_DrawPixel(2, 2, 1);
    LCD_DrawPixel(3, 2, 1);
    LCD_DrawPixel(2, 3, 1);
    LCD_DrawPixel(3, 3, 1);

    LCD_DrawPixel(124, 2, 1);
    LCD_DrawPixel(125, 2, 1);
    LCD_DrawPixel(124, 3, 1);
    LCD_DrawPixel(125, 3, 1);

    LCD_DrawPixel(2, 60, 1);
    LCD_DrawPixel(3, 60, 1);
    LCD_DrawPixel(2, 61, 1);
    LCD_DrawPixel(3, 61, 1);

    LCD_DrawPixel(124, 60, 1);
    LCD_DrawPixel(125, 60, 1);
    LCD_DrawPixel(124, 61, 1);
    LCD_DrawPixel(125, 61, 1);

    LCD_UpdateFullScreen();
}

void LCD_ShowAlphabetTest(void)
{
    /* Limpiar buffer antes de la prueba de texto */
    SC5260_ClearArea(0, 0, 128, 64, 0);

    /* Textos requeridos en distintos tamaños */
    /* 1. Título del protocolo: Tamaño Large (3x) - 15x21 pixels per char */
    /* "VRFR" cabe en 128px (4 * 18 = 72px) */
    LCD_DrawIcon(ICON_LAMBDA, 2, 2);
    UI_DrawText(14, 2, "VRFR", SCALE_LARGE);

    /* 2. Estado: Tamaño Normal (2x) - 10x14 pixels per char */
    /* "ARMED" */
    LCD_DrawIcon(ICON_FLAG_BLACK, 2, 26);
    UI_DrawText(14, 26, "ARMED", SCALE_NORMAL);

    /* 3. Datos técnicos: Tamaño Tiny (1x) - 5x7 pixels per char */
    /* "BAT: 8.4V" */
    UI_DrawText(2, 44, "BAT:8.4", SCALE_TINY);

    LCD_DrawIcon(ICON_ENVELOPE, 46, 43);
    UI_DrawText(56, 44, "MSG", SCALE_TINY);

    LCD_DrawIcon(ICON_MAP, 78, 43);
    UI_DrawText(88, 44, "LOC", SCALE_TINY);

    /* 4. Iconografía Táctica (8x8) */
    LCD_DrawIcon(ICON_VFO, 2, 54);
    UI_DrawText(12, 55, "VFO", SCALE_TINY);

    LCD_DrawIcon(ICON_TX, 36, 54);
    UI_DrawText(46, 55, "TX", SCALE_TINY);

    LCD_DrawIcon(ICON_SCAN, 66, 54);
    UI_DrawText(76, 55, "SCN", SCALE_TINY);

    LCD_DrawIcon(ICON_SKULL, 98, 54);
    UI_DrawText(108, 55, "PIR", SCALE_TINY);

    /* Enviar al LCD */
    LCD_UpdateFullScreen();
}
