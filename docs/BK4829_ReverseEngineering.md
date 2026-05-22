# Ingeniería Inversa del BK4829 (Baofeng UV-K6)

Este documento documenta los hallazgos críticos al intentar inicializar el chip de RF Beken BK4829 "desde cero" (baremetal), descubriendo el funcionamiento de registros vitales que no están documentados públicamente.

## El Problema del "Squelch Mudo"
Durante el desarrollo del firmware Custom Half-Life, el chip BK4829 reportaba que el PLL estaba enganchado (`0x01` con bit `0x0002` en alto) y los comandos SPI funcionaban correctamente. Sin embargo, no había salida de audio analógico (ni estática, ni ruido blanco, ni tonos).

Se diagnosticaron dos problemas principales en la gestión de energía y la lógica de Squelch del chip:

### 1. El Registro 0x30 (Modos RX/TX y LDOs)
Se asumía (por comentarios engañosos en código legado) que para encender el receptor o generar un tono bastaba con escribir `0x0002` en el registro `0x30`. Esto fue un error catastrófico.

*   **El problema:** El registro `0x30` no solo controla el modo RX/TX, sino que también contiene los bits de habilitación para el Reloj de Referencia (`XTAL_ENABLE`) y los múltiples reguladores LDO internos que alimentan las capas analógicas (DAC, Squelch, AFOUT). Al escribir `0x0002`, se asfixiaba al chip energéticamente.
*   **La solución (Valores Nativos):** El firmware original escribe valores compuestos complejos.
    *   **Modo RX (`0xBFF1`):** Habilita todos los LDOs, el reloj, y engancha el receptor.
    *   **Modo TX (`0xC1FE`):** Prepara la cadena de modulación.
    *   **Modo TONE (`0x0302`):** Activa el generador de tonos local sin habilitar la portadora de transmisión de RF.

### 2. El Registro 0x78 (Squelch Principal) y 0x48 (Mute/Volumen)
Había confusión entre el registro de control de Squelch y el registro de Mute/DAC.

*   **Registro `0x48`:** Controla la habilitación del DAC de salida (bit `0x8000`) y el volumen (bits `13:4`). No controla el Squelch en sí.
*   **Registro `0x78`:** Es la compuerta de Squelch real. Si este registro no es `0x0000`, la compuerta permanece cerrada a menos que haya una portadora válida superando el umbral.
*   **El problema:** Aunque forzábamos el volumen al máximo (`0x83F2` en `0x48`), el Squelch en `0x78` estaba en un valor por defecto que mantenía enmudecido el enrutamiento interno (`AFOUT`).
*   **La solución:** Para abrir completamente el piso de ruido (y escuchar la estática), es obligatorio escribir `0x0000` en `0x78` además de habilitar el DAC en `0x48`. Para cerrar el Squelch, se escribe un umbral como `0x3040`.

## Secuencia Correcta de Inicialización Analógica
Para forzar al chip a emitir sonido hacia la bocina, la secuencia requerida es:

1.  Habilitar el amplificador físico (`GPIOB_Pin_2` = ALTO).
2.  Habilitar el Bias de Audio (`GPIOA_Pin_3` = ALTO).
3.  Forzar Squelch abierto (`BK4829_WriteReg(0x78, 0x0000)`).
4.  Encender DAC y Volumen (`BK4829_WriteReg(0x48, 0x83F2)`).
5.  Enrutar AFOUT RX hacia DAC (`BK4829_WriteReg(0x47, 0x6142)`).
6.  Encender chip en Modo RX con LDOs (`BK4829_WriteReg(0x30, 0xBFF1)`).

### 3. El Registro 0x47 (Enrutamiento AFOUT) y 0x31 (AGC)
Durante las pruebas del "Minifirm", descubrimos que no basta con encender el DAC y el amplificador.
*   **Registro 0x47:** El bit 8 es un interruptor físico maestro (`RX AFOUT`). Escribir `0x6042` enciende funciones analógicas, pero el audio proveniente del demodulador interno se corta. Escribir `0x6142` "conecta el cable interno" desde el receptor hacia el DAC de audio.
*   **Registro 0x31:** Un valor incorrecto como `0x8206` silencia la ganancia del AGC. El valor correcto comprobado en capturas ("Golden Sequence") es `0xFFFD`. Además, es crítico inicializar la tabla de curvas del AGC en las 16 direcciones del registro `0x09`.

### 4. Tuning y Sincronización de Variables Globales (0x38 / 0x39)
Un comportamiento inesperado fue que la frecuencia de recepción (`0x38` LSB y `0x39` MSB) registraba `0x0000` a pesar de estar asignada a una variable global `g_test_freq = 43305000;`. 
*   **La causa:** En ramas experimentales "Baremetal", si el script de arranque no copia correctamente la sección `.data` desde la Flash a la SRAM antes de inicializar el SPI, las variables pre-inicializadas se quedan en 0. 
*   **La solución:** Hasta que la tabla de vectores y relocalización de SRAM esté 100% estable, las frecuencias clave (`0xC83D` y `0x0294`) deben pasarse como literales o mediante macros `#define` (ej: `0xC83D` LSB y `0x0294` MSB para 433.050 MHz) durante la inicialización para evitar la sintonía accidental en 0 MHz.
