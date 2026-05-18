#include "includes.h"

extern void UI_DisplayPowerOn(void)
{
    U8 i;

    // Inicializar el búfer de pantalla a blanco/limpio
    LCD_ClearFullBuf();

    // Establecer la relación de contraste (3 = nivel óptimo 48)
    SC5620_SetContpastRatio(g_radioInform.brightness);

    // Dibujar marco industrial doble estilo retro Valve/Half-Life (ahora visible gracias al fix en LcdFillDot.c!)
    LCD_DrawRectangle(0, 0, 128, 64, 0);
    LCD_DrawRectangle(2, 2, 124, 60, 0);

    // Dibujar barra central de fondo invertido (negro sólido)
    LCD_DrawRectangle(15, 6, 116, 18, 1);

    // Mostrar el título "HALF-LIFE" centrado e invertido dentro de la barra
    LCD_DisplayText(16, 28, (U8 *)"HALF-LIFE", FONTSIZE_16x16, LCD_DIS_INVERT);

    // Subtítulo del ecosistema "ROIP ECOSYSTEM" en modo normal (negro sobre fondo claro)
    LCD_DisplayText(36, 22, (U8 *)"ROIP ECOSYSTEM", FONTSIZE_12x12, LCD_DIS_NORMAL);

    // Micro-animación premium: Marco exterior de la barra de progreso de carga en la parte inferior
    LCD_DrawRectangle(51, 34, 60, 5, 0);

    // Mostrar el diseño estático inicial en la pantalla física para evitar parpadeos
    LCD_UpdateFullScreen();

    // Estabilización eléctrica: Encender la luz de fondo después del primer renderizado
    LcdBackLightSwitch(LED_ON);

    // Carga progresiva de la barra (animación fluida de 8 pasos, 56 píxeles de longitud máxima)
    for (i = 0; i <= 56; i += 8)
    {
        if (i > 0)
        {
            LCD_DrawRectangle(52, 35, i, 3, 1);
            LCD_UpdateFullScreen();
        }
        DelayMs(60);
    }

    // Delay de persistencia final para poder contemplar la animación y el logotipo
    DelayMs(200);
}
