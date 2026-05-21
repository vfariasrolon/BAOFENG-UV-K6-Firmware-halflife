# UI Layout y Diccionario Visual (UV-K6)

Este documento centraliza el mapa de la interfaz de usuario, resoluciones, ubicaciones de memoria y el diccionario de iconos tácticos del sistema.

## 1. Mapa de Coordenadas (128x64 píxeles)

La pantalla SC5260 tiene una resolución de 128x64 píxeles. Debido a la relación de aspecto del cristal físico, los píxeles son más altos que anchos (relación aprox. 1.33:1).

| Región | Coordenadas Y | Altura | Uso Principal |
| :--- | :--- | :--- | :--- |
| **Header** | `0` a `23` | 24 px | Título del protocolo (`SCALE_LARGE` 15x21) o VFO A |
| **Status** | `26` a `40` | 15 px | Estado del sistema (`SCALE_NORMAL` 10x14) o VFO B |
| **Data Row** | `43` a `51` | 9 px | Datos técnicos, voltaje, GPS (`SCALE_TINY` 5x7) |
| **Tactical Row**| `54` a `63` | 10 px | Iconos tácticos (8x8) y etiquetas. |

## 2. Diccionario de Iconografía (`icons.h`)

Todos los iconos se renderizan utilizando la función `LCD_DrawIcon(IconID_Enum iconID, U8 x, U8 y)`. 
Tienen un tamaño de **8x8 píxeles** para preservar legibilidad sin ocupar demasiado espacio.

| `IconID_Enum` | Descripción | Ubicación recomendada |
| :--- | :--- | :--- |
| `ICON_VFO` | Cuadro con 'V' para el modo VFO | Tactical Row |
| `ICON_MR` | Cuadro con 'M' para el modo MR (Memory) | Tactical Row |
| `ICON_FM` | Nota musical para el receptor FM | Tactical Row |
| `ICON_TX` | Antena emitiendo ondas (Transmisión) | Tactical Row |
| `ICON_RX` | Altavoz con señal (Recepción) | Tactical Row |
| `ICON_SCAN` | Flechas circulares (Escaneo activo) | Tactical Row |
| `ICON_BAT_0` | Batería vacía (Marco) | Tactical Row (extremo derecho) |
| `ICON_BAT_50` | Batería a la mitad | Tactical Row (extremo derecho) |
| `ICON_BAT_100` | Batería completamente llena | Tactical Row (extremo derecho) |
| `ICON_SKULL` | Cráneo pirata 8-bit (Modo sigilo/fuego) | Tactical Row o Data Row |
| `ICON_FLAG_WHITE`| Bandera Blanca (Paz / Escucha) | Status Row / Tactical Row |
| `ICON_FLAG_BLACK`| Bandera Negra (Combate / Armado) | Status Row / Tactical Row |
| `ICON_ENVELOPE` | Sobre cerrado (Mensajes / Telemetría) | Data Row |
| `ICON_MAP` | Mapa doblado con un punto X (Coordenadas) | Data Row |
| `ICON_LAMBDA` | Símbolo Lambda en círculo (Protocolo HL) | Header (Logo) |

## 3. Motor Tipográfico (`minifont.c`)

El motor utiliza una fuente base de 5x7 píxeles y la escala en tiempo real mediante multiplicación de píxeles:

*   `SCALE_TINY` (1x): 5x7 píxeles por carácter. Ideal para la zona de datos técnicos (Data Row).
*   `SCALE_NORMAL` (2x): 10x14 píxeles. Ideal para nombres de canales o estados secundarios.
*   `SCALE_LARGE` (3x): 15x21 píxeles. Reservado para el protocolo principal o frecuencias.

## 4. Jerarquía de Estados FSM (`AppEventManager.c`)

La pantalla reacciona a los siguientes estados globales definidos en `UI_State_Enum`:

*   `UI_STATE_MAIN`: Vista principal del protocolo VRFR.
*   `UI_STATE_DEBUG_MAPPING`: Muestra la tecla presionada en gran tamaño en el centro de la pantalla. Limpia toda el área central.
*   *(Futuro)* `UI_STATE_MENU`: Vista en lista utilizando `SCALE_NORMAL` con un cursor invertido.

## 5. Motor de Renderizado Gráfico (Logotipos e Imágenes)

Para inyectar imágenes de pantalla completa (128x64) como el "Boot Logo" de Half-Life, el firmware utiliza la función `LCD_DrawLogo()`, la cual actúa como un traductor entre un array bidimensional en C y la memoria VRAM paginada del chip SC5260 (o ST7567).

### Especificaciones de la Matriz de Imagen
Para que una imagen se dibuje perfectamente en la pantalla sin rotaciones, distorsiones diagonales, o efecto espejo, debe generarse respetando estrictamente estas reglas:
*   **Tamaño Exacto:** 128x64 píxeles (array unidimensional de exactamente 1024 bytes). No requiere bytes de cabecera ni padding.
*   **Modo de Escaneo (Scan Mode):** Horizontal Puro, fila por fila. Se leen primero los primeros 16 bytes de la fila Y=0, luego los 16 de la fila Y=1, y así sucesivamente.
*   **Orden de Bits (Endianness):** MSB First (Most Significant Bit First). El píxel más a la izquierda de un bloque de 8 píxeles está representado por el bit `0x80` (128), y el de la derecha por `0x01` (1).
*   **Colorimetría:** `1` representa el "Fondo / Píxel Apagado" (color verde de la retroiluminación), mientras que `0` representa un "Píxel Encendido" (negro opaco del cristal).

### Supersampling (Script `img2c.py`)
Para evitar artefactos visuales, el script conversor `img2c.py` utiliza una técnica de *Supersampling*. Dibuja el logo a 4 veces la resolución de la pantalla en escala de grises (512x256) aprovechando el suavizado (Anti-Aliasing) de las primitivas geométricas, y posteriormente reduce matemáticamente la imagen aplicando un umbral (Threshold). Esto elimina por completo el infame patrón de rayas (striping / dithering) generado al dibujar líneas diagonales gruesas en pantallas nativas de 1-bit.
