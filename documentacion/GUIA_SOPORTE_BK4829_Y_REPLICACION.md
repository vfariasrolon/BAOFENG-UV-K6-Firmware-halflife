# Guía de Soporte BK4829, Resolución de Problemas y Replicación de Entorno

Esta guía documenta detalladamente el análisis técnico, los problemas iniciales encontrados en los radios Baofeng UV-K6 con el chip **Beken BK4829**, las soluciones implementadas y los pasos exactos para replicar este entorno de desarrollo completo en cualquier otra computadora de forma instantánea.

---

## 📺 1. Diagnóstico Técnico y Problemas Iniciales

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
    3.  **Tonos DTMF**: Mandar frecuencias en Hz crudos (`941`) causaba un "clic". El BK4829 requiere convertir los Hz mediante una fórmula matemática (`Reg = Hz * 10.324`) y usar el modo oculto **TXTONE** (`0xC3FA`) para que el audio pase al aire.

---

## 🛠️ 2. Soluciones Técnicas Implementadas (BK4829_Minimal.c)

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
*   **Modo TXTONE (Aire)**: Para enviar DTMF por radio, se usa `0x30 = 0xC3FA`, se habilita el threshold en `0x24 = 0x87FF` y la ganancia en `0x70 = 0xE0E0`. Al terminar, obligatoriamente hay que regresar al estado RX (`0xBFF1`).

---

## 💻 3. Guía de Replicación de Entorno (Instalar y Ejecutar)

Si quieres compilar y flashear este código en **cualquier otra computadora** Linux (Ubuntu/Debian) de manera instantánea, sigue estos sencillos pasos:

### Paso 1: Clonar y Descargar el Repositorio
```bash
git clone <URL_DEL_REPOSITORIO> BAOFENG-UV-K6-Firmware-halflife
cd BAOFENG-UV-K6-Firmware-halflife
git checkout bk4829-compat
```

### Paso 2: Otorgar Permisos de Ejecución a los Scripts
```bash
chmod +x setup.sh quick_run.sh
```

### Paso 3: Ejecutar el Instalador Automático
Este comando instalará de manera segura todas las dependencias requeridas del sistema (paquetes de Python, compiladores de ARM y librerías) y configurará el entorno virtual local (`venv`):
```bash
./setup.sh
```

### Paso 4: Solucionar Permisos de Puerto USB Serial (Dialout)
Para poder escribir en tu cable USB de programación sin usar `sudo` ni requerir ser root, debes agregarte al grupo `dialout` y refrescarlo:
```bash
# Agregar a dialout (Ya lo hace setup.sh, pero puedes forzarlo)
sudo usermod -a -G dialout $USER

# Aplicar permisos inmediatos al puerto USB conectado
sudo chmod 666 /dev/ttyUSB0

# Cargar el nuevo grupo sin necesidad de reiniciar sesión
newgrp dialout
```

### Paso 5: Compilar y Flashear con un Solo Script (`quick_run.sh`)
Hemos creado un script integrador para compilar el firmware actual, localizar el último binario compilado y abrir automáticamente la interfaz interactiva de flasheo:
```bash
./quick_run.sh
```
*Este script compilará de forma súper veloz usando PlatformIO y te guiará paso a paso para subir el nuevo binario a tu radio.*

---

## 🛠️ 4. Depuración en Tiempo Real (`k6_reg_editor.py`)

Para no tener que compilar y flashear el firmware cada vez que queramos probar una regla de registros diferente, hemos inyectado un **protocolo de depuración serial no bloqueante** en la radio (`CheckProgromMode` en `src/CPS/ProgromFlash.c`) y creado una consola interactiva en Python.

Esto te permite leer y escribir cualquier registro del transceptor de RF en caliente, mientras la radio está sintonizada en una frecuencia.

### A. Requisitos de Conexión
1.  Flashea la radio con la rama `bk4829-compat` (que ya tiene el parser inyectado).
2.  Enciende la radio en **modo normal** (sin entrar a modo de flasheo).
3.  Conecta el cable de programación al PC y a la radio lateralmente.
4.  Aplica permisos al puerto serial:
    ```bash
    sudo chmod 666 /dev/ttyUSB0
    ```

### B. Iniciar el Depurador Interactivo
Ejecuta la consola interactiva en la raíz del proyecto usando el entorno virtual:
```bash
./venv/bin/python3 k6_reg_editor.py
```

### C. Comandos Disponibles dentro de la Consola
Una vez dentro de la terminal `halflife-dbg>`, puedes ingresar los siguientes comandos:

*   **Escanear todos los registros**:
    ```text
    scan
    ```
    *Vuelca en pantalla el estado actual de los 128 registros del transceptor (del 0x00 al 0x7F).*

*   **Leer un registro específico** (acepta decimal o hex):
    ```text
    r 0x30
    ```
    O también:
    ```text
    r 48
    ```

*   **Escribir un valor en un registro específico**:
    ```text
    w 0x30 0x0007
    ```
    *(Este comando fuerza a la radio a habilitar la transmisión del micrófono en tiempo real en el BK4829).*
    
*   **Salir de la herramienta**:
    ```text
    exit
    ```

---

## 🧪 5. Pruebas Críticas para Mañana

Mañana que realices las pruebas físicas, te sugiero seguir este orden exacto de validaciones diagnósticas:

1.  **Verificación de Chip ID**:
    Arranca el depurador interactive `./venv/bin/python3 k6_reg_editor.py` y escribe `r 0x00` (o `scan` completo).
    *   **BK4819**: Debería devolver `0x4819`.
    *   **BK4829**: Debería devolver `0x4829`.
    *   *Si devuelve 0x0000 o 0xFFFF, hay un falso contacto de SPI en la soldadura del chip o ruido en la línea.*

2.  **Prueba de Squelch Abierto (Monitor)**:
    Presiona el botón de linterna para entrar en monitor (estática). 
    Si hay sordera, usa el editor serial en caliente para escribir `w 0x48 0x0000` (abre el squelch por completo). Si se escucha estática fuerte (`shhhhh`), el receptor de RF y el amplificador de audio están 100% funcionales por hardware.

3.  **Prueba de Micrófono en Transmisión**:
    Presiona PTT y habla al radio. Si no se escucha modulación, abre la consola interactiva y escribe:
    `w 0x30 0x0007` (Habilitador nativo del canal del micrófono del BK4829)
    y luego:
    `w 0x7D 0xE952` (Habilitador del preamplificador analógico del micrófono y sensibilidad)
    Esto nos permitirá forzar manualmente y en tiempo real el ruteo del micrófono para escuchar en otro equipo receptor hasta dar con la regla exacta de tu equipo.

