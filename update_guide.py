import re

with open("documentacion/GUIA_SOPORTE_BK4829_Y_REPLICACION.md", "r") as f:
    content = f.read()

# Replace Section 1
new_section1 = """## 📺 1. Diagnóstico Técnico y Problemas Iniciales

Los radios UV-K6 de revisiones recientes han reemplazado el transceptor de RF Beken BK4819 por el nuevo **BK4829**. Al cargarles un firmware diseñado únicamente para el chip viejo, se presentaban tres fallos críticos debido a diferencias estructurales en el mapa de registros:

### A. Bloqueo y Congelamiento de la CPU
*   **Problema**: Al inicializar, la radio se bloqueaba o se quedaba sorda con un "clic seco".
*   **Causa**: El driver del BK4819 inicializaba registros (como `0x1C` a `0x1F`, `0x28` a `0x2F`, etc.) que en el **BK4829** corrompen el estado interno del chip. El BK4829 requiere una secuencia estricta de reinicio y configuración de los LDOs (`0x37` y `0x36`) antes de estabilizar el PLL.

### B. "Sordera" Total de Recepción (Sin Audio Físico)
*   **Problema**: La radio recibía señal (el LED encendía), pero permanecía en silencio absoluto.
*   **Causa**: Descubrimos que la placa física requiere pines de la MCU para habilitar el audio que no estaban siendo activados:
    1.  **Amplificador de Bocina**: El pin `GPIOB_Pin_2` debe estar en ALTO (`BSRR`) para dar energía al amplificador.
    2.  **Bias de Audio RX**: El pin `GPIOA_Pin_3` debe estar en ALTO para abrir el paso analógico de recepción.
    3.  **Conflicto del Squelch (`0x48`)**: En el BK4819 `0x48` controla el volumen, pero en el BK4829 es el Registro Maestro del Squelch.

### C. Falla de Transmisión (Sin Potencia RF y Sin Modulación)
*   **Problema**: Al transmitir (PTT), el radio indicaba TX pero no emitía RF real ni audio.
*   **Causa**:
    1.  **Amplificador de Potencia (PA)**: El firmware viejo encendía el PA a través del registro `0x3B`. En el BK4829, el PA se controla escribiendo en el **registro `0x36`** (ej. `0x01FF` para encender, `0x007F` para apagar).
    2.  **Estados TX/RX (`0x30`)**: Estábamos usando estados simples (`0x0002` y `0x0003`). El código fuente nativo del BK4829 requiere tramas complejas: `0xBFF1` para RX y `0xC1FE` para TX.
    3.  **Tonos DTMF**: Mandar frecuencias en Hz crudos (`941`) causaba un "clic". El BK4829 requiere convertir los Hz mediante una fórmula matemática (`Reg = Hz * 10.324`) y usar el modo oculto **TXTONE** (`0xC3FA`) para que el audio pase al aire."""

# Replace Section 2
new_section2 = """## 🛠️ 2. Soluciones Técnicas Implementadas (BK4829_Minimal.c)

Hemos desarrollado un driver minimalista y 100% estable (`BK4829_Minimal.c`) que soluciona de raíz todos los bloqueos físicos y lógicos de RF:

### A. Habilitación de Hardware Físico (Pines MCU)
Añadimos la configuración de pines GPIO al inicio para revivir el hardware analógico de la placa:
*   `GPIOB_Pin_2 = ON`: Enciende el transistor del amplificador de la bocina. Sin esto, el equipo estaba completamente mudo.
*   `GPIOA_Pin_3 = ON`: Habilita el *bias* de audio RX.
*   **Gestión del Squelch**: `BK4829_SetAudioMute(true)` escribe `0x2340` en `0x48` (cerrar squelch) y **apaga el pin B2**, asegurando cero ruido residual. Al desmutear, escribe `0x0000` y enciende la bocina.

### B. Inicialización Limpia de RF
Se omiten las escrituras destructivas y se configura estrictamente la secuencia de encendido LDO/Band Gap necesaria:
1. Reset por software (`0x00 = 0x0000`).
2. Habilitación de LDO y Reloj (`0x37 = 0x9F1F`, `0x36 = 0x0022`).
3. Valores de AGC, Audio y PLL recuperados del código fuente nativo de fábrica.
4. Estado en recepción inactivo (`0x30 = 0xBFF1`).

### C. Control de Transmisión Real (El Secreto de 0x36)
La transición a TX (`BK4829_TxEnable`) ahora aplica los tres pasos obligatorios:
1.  **Conmutación RF Externa**: `GPIOA_Pin_13` y `Pin_14` en LOW para activar la ruta UHF de TX en los switches de placa.
2.  **Conmutación RF Interna**: Se re-configuran los GPIOs internos del BK4829 vía registro `0x33` (`RF_GPIO3=LOW`, `RF_GPIO2=HIGH`).
3.  **Encendido del PA (Power Amplifier)**: Se escribe `0x01FF` en el **registro `0x36`**. (El apagado escribe `0x007F`).
4.  **Estado TX**: Se escribe `0xC1FE` en el registro `0x30`.

### D. Modulación y Frecuencias DTMF Matemáticas
Para emitir pitidos y tonos DTMF funcionales:
*   **Fórmula de Conversión**: El BK4829 usa un formato de registro. Para pasar de Hz crudos a Registro, usamos la macro: `#define BK4829_HZ_TO_REG(hz) ((hz) * 1032444UL / 100000UL)`. 
*   **Modo TONE Local**: Para el boot beep, usamos `0x30 = 0x0302`, que reproduce el tono en la bocina sin transmitir portadora RF.
*   **Modo TXTONE (Aire)**: Para enviar DTMF por radio, se usa `0x30 = 0xC3FA`, se habilita el threshold en `0x24 = 0x87FF` y la ganancia en `0x70 = 0xE0E0`. Al terminar, obligatoriamente hay que regresar al estado RX (`0xBFF1`)."""

content = re.sub(r'## 📺 1\. Diagnóstico Técnico y Problemas Iniciales.*?---', new_section1 + '\n\n---', content, flags=re.DOTALL)
content = re.sub(r'## 🛠️ 2\. Soluciones Técnicas Implementadas.*?---', new_section2 + '\n\n---', content, flags=re.DOTALL)

with open("documentacion/GUIA_SOPORTE_BK4829_Y_REPLICACION.md", "w") as f:
    f.write(content)
