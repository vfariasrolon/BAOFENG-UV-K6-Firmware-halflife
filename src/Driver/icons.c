#include "icons.h"
#include "Sc5260.h"

/* Bitmaps de 8x8 píxeles (1 byte por fila, 8 filas) */
static const U8 IconBitmaps[ICON_MAX][8] = {
    [ICON_VFO] = {
        0xFF, // 11111111
        0x81, // 10000001
        0xDD, // 11011101
        0xDD, // 11011101
        0xEB, // 11101011
        0xF7, // 11110111
        0x81, // 10000001
        0xFF  // 11111111
    },
    [ICON_MR] = {
        0xFF, // 11111111
        0x81, // 10000001
        0x99, // 10011001
        0xAA, // 10101010
        0xC3, // 11000011
        0xC3, // 11000011
        0x81, // 10000001
        0xFF  // 11111111
    },
    [ICON_FM] = {
        0x0E, // 00001110 (Nota musical)
        0x0B, // 00001011
        0x09, // 00001001
        0x09, // 00001001
        0x19, // 00011001
        0x39, // 00111001
        0x78, // 01111000
        0x30  // 00110000
    },
    [ICON_TX] = {
        0x18, // 00011000 (Antena con ondas)
        0x5A, // 01011010
        0x99, // 10011001
        0x18, // 00011000
        0x18, // 00011000
        0x18, // 00011000
        0x18, // 00011000
        0x3C  // 00111100
    },
    [ICON_RX] = {
        0x10, // 00010000 (Altavoz con sonido)
        0x34, // 00110100
        0x7A, // 01111010
        0x79, // 01111001
        0x7A, // 01111010
        0x34, // 00110100
        0x10, // 00010000
        0x00  // 00000000
    },
    [ICON_SCAN] = {
        0x3C, // 00111100 (Flechas rotando)
        0x42, // 01000010
        0x99, // 10011001
        0xA5, // 10100101
        0xA5, // 10100101
        0x99, // 10011001
        0x42, // 01000010
        0x3C  // 00111100
    },
    [ICON_BAT_0] = {
        0x3C, // 00111100 (Terminal)
        0x42, // 01000010 (Tope)
        0x81, // 10000001 (Vacío)
        0x81, // 10000001
        0x81, // 10000001
        0x81, // 10000001
        0x81, // 10000001
        0xFF  // 11111111 (Base)
    },
    [ICON_BAT_25] = {
        0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0xFF, 0xFF
    },
    [ICON_BAT_50] = {
        0x3C, 0x42, 0x81, 0x81, 0xFF, 0xFF, 0xFF, 0xFF
    },
    [ICON_BAT_75] = {
        0x3C, 0x42, 0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    [ICON_BAT_100] = {
        0x3C, 0x42, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    [ICON_SKULL] = {
        0x7E, // 01111110 (Cráneo top)
        0xFF, // 11111111 (Frente)
        0xDB, // 11011011 (Ojos huecos)
        0xDB, // 11011011 (Ojos huecos)
        0xE7, // 11100111 (Nariz)
        0x7E, // 01111110 (Maxilar)
        0x3C, // 00111100 (Dientes)
        0x3C  // 00111100 (Dientes)
    },
    [ICON_FLAG_WHITE] = {
        0x80, // 10000000 (Mástil)
        0xFE, // 11111110 (Borde superior)
        0x82, // 10000010 (Interior vacío)
        0x82, // 10000010
        0xFE, // 11111110 (Borde inferior)
        0x80, // 10000000 (Mástil)
        0x80, // 10000000 (Mástil)
        0x80  // 10000000 (Mástil)
    },
    [ICON_FLAG_BLACK] = {
        0x80, // 10000000
        0xFE, // 11111110
        0xFE, // 11111110 (Interior lleno)
        0xFE, // 11111110
        0xFE, // 11111110
        0x80, // 10000000
        0x80, // 10000000
        0x80  // 10000000
    },
    [ICON_ENVELOPE] = {
        0x00, // 00000000
        0xFF, // 11111111 (Borde superior)
        0xC3, // 11000011 (Solapa bajando)
        0xA5, // 10100101 (Diagonal)
        0x99, // 10011001 (Centro solapa)
        0x81, // 10000001
        0xFF, // 11111111 (Borde inferior)
        0x00  // 00000000
    },
    [ICON_MAP] = {
        0xFF, // 11111111 (Borde mapa)
        0x81, // 10000001
        0x85, // 10000101 (Camino)
        0x81, // 10000001
        0x8A, // 10001010 (Camino)
        0x91, // 10010001 (Camino)
        0xA1, // 10100001 (Punto X)
        0xFF  // 11111111 (Borde mapa)
    },
    [ICON_LAMBDA] = {
        0x3C, // 00111100 (Círculo top)
        0x42, // 01000010
        0x89, // 10001001 (Lambda top)
        0x91, // 10010001 (Pierna derecha bajando)
        0xA1, // 10100001 (Pierna izquierda)
        0xC1, // 11000001 (Base lambda)
        0x42, // 01000010
        0x3C  // 00111100 (Círculo bot)
    }
};

void LCD_DrawIcon(IconID_Enum iconID, U8 x, U8 y)
{
    U8 row, col, bits;

    if (iconID >= ICON_MAX) return;

    for (row = 0; row < 8; row++) {
        bits = IconBitmaps[iconID][row];
        for (col = 0; col < 8; col++) {
            /* Leer bit desde MSB a LSB para dibujar de izquierda a derecha */
            if (bits & (0x80 >> col)) {
                LCD_DrawPixel(x + col, y + row, 1);
            }
        }
    }
}
