# Guía de Soporte BK4829, Resolución de Problemas y Replicación de Entorno

Esta guía documenta detalladamente el análisis técnico, los problemas iniciales encontrados en los radios Baofeng UV-K6 con el chip **Beken BK4829**, las soluciones implementadas y los pasos exactos para replicar este entorno de desarrollo completo en cualquier otra computadora de forma instantánea.

---

## 📺 1. Diagnóstico Técnico y Problemas Iniciales

Los radios UV-K6 de revisiones recientes han reemplazado el transceptor de RF Beken BK4819 por el nuevo **BK4829**. Al cargarles un firmware diseñado únicamente para el chip viejo, se presentaban tres fallos críticos debido a diferencias estructurales en el mapa de registros:

### A. Bloqueo y Congelamiento de la CPU (PTT / Linterna-Monitor)
*   **Problema**: Al presionar PTT o intentar abrir el Squelch con la linterna (Monitor), el procesador se congelaba por completo y la radio se bloqueaba.
*   **Causa**: El driver del BK4819 controlaba la máquina de estados escribiendo tramas de control complejas en el **registro `0x30`** (como `0xBFF1` para RX y `0xC1FE` para TX). En el **BK4829, el registro `0x30` es un registro simple de estado**: espera estrictamente `0x0002` para habilitar recepción (RX) y `0x0003` para transmisión (TX). Mandar los valores viejos bloqueaba internamente el secuenciador del chip, deteniendo el reloj y congelando la CPU de la radio.

### B. "Sordera" Total de Recepción (Squelch Herméticamente Cerrado)
*   **Problema**: La radio permanecía en silencio absoluto. Ninguna señal analógica abría el Squelch, y los beeps sonaban sumamente apagados (como clics secos digitales).
*   **Causa**:
    1.  **Conflicto de Registros (`0x48`)**: En el BK4819, el registro `0x48` controla la ganancia del DAC (volumen). Pero en el **BK4829, el registro `0x48` es el Registro Maestro del Squelch**. Cada vez que el firmware intentaba ajustar el volumen de audio de RX o Beeps escribiendo en `0x48`, sobrescribía el umbral de Squelch del BK4829 con valores muy altos, forzándolo a cerrarse permanentemente.
    2.  **Calibración Desactivada**: La calibración dinámica del cristal de 26 MHz (`XTAL_ADJUST`) estaba forzada a un valor fijo genérico (`8`), ignorando la calibración real de fábrica guardada en la EEPROM de tu placa de circuito. Al no tener el offset exacto, el PLL se desfasaba de frecuencia, cerrando por completo el squelch angosto de FM.

### C. Falla de Modulación en Transmisión (PTT sin Audio / Transmisión Falsa)
*   **Problema**: Al transmitir (PTT), el indicador salía en pantalla y el nivel de señal subía al máximo, pero el receptor del otro lado no escuchaba nada de audio (portadora silenciosa).
*   **Causa**:
    1.  **Falsa Transmisión Continua**: En `Rfic_GetRxTxState()`, el software evaluaba el estado leyendo el registro `0x30` y verificando `temp & 0x0002` para saber si estaba en TX. Sin embargo, en el BK4829 la recepción (RX) activa el valor `0x0002`. Por lo tanto, `temp & 0x0002` daba verdadero **siempre**, haciendo creer a la CPU que la radio estaba transmitiendo todo el tiempo, desconfigurando los ciclos de audio.
    2.  **Corrupción del Estado del Micrófono**: En `Rfic_MicIn_Enable()` y `Rfic_MicIn_Disable()`, el código original modificaba el registro `0x30` aplicando una máscara de bits `| 0x0004` para activar el micrófono. En el BK4829, alterar este bit modificaba el estado de `0x0003` (TX) a `0x0007`, corrompiendo la modulación y cancelando la entrada del micrófono.

---

## 🛠️ 2. Soluciones Técnicas Implementadas

Hemos desarrollado una capa de compatibilidad inteligente en `src/Driver/DevFD6818.c` y `src/Driver/RadioDataStorage.c` para que el firmware identifique automáticamente el chip y se comporte según corresponda:

### A. Detección Inteligente del Silicio
En `Rfic_Init()`, leemos el ID de hardware del chip en el bus SPI:
```c
U16 chipID = Rfic_ReadWord(0);
if (chipID == 0x4829) {
    g_isBK4829 = 1;
}
```
Si se detecta el chip BK4829, se inicializan únicamente sus registros maestros nativos:
*   `0x00` -> `0x0000` (Reset por software)
*   `0x01` -> `0x3FF0` (Referencia de reloj estable)
*   `0x48` -> `0x2340` (Squelch inicial por defecto)
*   `0x70` -> `0x00E0` (Ganancia LNA e Intermedia)
*   `0x74` -> `0x3B2D` (Filtros pasa-banda de audio analógico)
*   `0x30` -> `0x0002` (Modo RX listo)

Y se omiten las escrituras a registros de BK4819 incompatibles (como las tablas de ganancia de AGC y DTMF) para evitar colgar el transceptor.

### B. Control Seguro de Estados (`0x30`) y Bypass de Micrófono
*   **En `Rfic_RxTxOnOffSetup`**: Si `g_isBK4829` es verdadero, se traduce la orden a tramas nativas de control:
    *   `RFIC_RXON` y `RFIC_TONE` -> Escribe `0x0002` en `0x30`.
    *   `RFIC_TXON` y `RFIC_TXTONE` -> Escribe `0x0003` en `0x30`.
    *   `RFIC_IDLE` -> Escribe `0x0000` en `0x30`.
*   **En Mic In**: En `Rfic_MicIn_Enable()` y `Rfic_MicIn_Disable()`, si `g_isBK4829` es verdadero, salimos inmediatamente sin tocar `0x30` para no corromper la modulación, ya que el estado nativo de transmisión `0x0003` habilita automáticamente la ruta de entrada de micrófono analógico en este silicio.

### C. Mapeo Dinámico del Squelch (`0x48`)
*   En `Rfic_SQLSetup()`, si se detecta el BK4829:
    *   Si se activa el modo Monitor (linterna) o Squelch `0`, se escribe `0x0000` en `0x48` para abrir por completo la recepción y reproducir estática analógica pura (`shhhhhh`).
    *   Para los niveles del 1 al 9, implementamos una tabla de atenuación de ruido digital lineal altamente optimizada:
        `{0x0000, 0x1540, 0x1B40, 0x2040, 0x2340, 0x2640, 0x2940, 0x2C40, 0x3040, 0x3440}`
*   En `Rfic_SetAfout()`, agregamos un guard para que si es el BK4829, se evite re-escribir ganancias del DAC en `0x48`, eliminando la causa raíz de la sordera por volumen.

### D. Discriminación de Estado RX/TX Correcta
En `Rfic_GetRxTxState()`, corregimos la lectura del registro `0x30` para el BK4829 evaluando el bit 0 (que indica modo TX cuando es `1`):
```c
if (g_isBK4829) {
    return (temp & 0x0001) ? 1 : 0;
}
```
Esto soluciona la retroalimentación falsa de transmisión y restaura los ciclos normales de demodulación analógica de audio.

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
Hemos creado un script integrador que compila de manera automatizada usando PlatformIO y lanza de forma automática el flasheador de rescate interactivo:
```bash
sudo ./quick_run.sh
```
*Este script compilará el firmware actual y abrirá la herramienta interactiva de rescate `k6_rescue.py` con privilegios elevados (`sudo`), guiándote paso a paso para seleccionar y grabar el binario sin conflictos de puerto.*

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

