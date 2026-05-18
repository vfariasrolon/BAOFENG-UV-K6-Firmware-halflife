# Guía de Ingeniería: Quansheng/Baofeng UV-K6 (KD32F328)

Este documento detalla la arquitectura de hardware del microcontrolador **KD32F328**, el protocolo de comunicación serial de su cargador de arranque (Bootloader), y los fundamentos de software que nos permitieron crear la interfaz unificada de **Half-Life** con total estabilidad y cero corrupción de pantalla.

---

## 1. Arquitectura de Hardware: Microcontrolador KD32F328

La radio **Baofeng UV-K6A / Quansheng UV-K5 / K6** está basada en el microcontrolador de fabricación china **KD32F328CBT6** (diseñado por *Kailing Micro* o *Kadi Microelectronics*).

### Especificaciones del Núcleo
* **Arquitectura:** ARM Cortex-M0 (32 bits).
* **Frecuencia de Reloj:** Corriendo típicamente a **96 MHz** mediante multiplicadores PLL internos.
* **Memoria Flash:** **128 KB** (mapeada en `0x08000000`). Aquí reside el firmware ejecutable.
* **SRAM:** **24 KB** (mapeada en `0x20000000`). Almacena las variables globales, la pila de llamadas (stack) y el buffer de la pantalla.
* **Periféricos Clave:**
  * **SPI:** Conectado directamente a la pantalla LCD y a la memoria EEPROM externa.
  * **ADC:** Para leer el nivel de voltaje de la batería, fuerza de la señal RSSI, y el micrófono.
  * **GPIO:** Manejo de botones laterales, PTT, y matriz de teclado numérico.
  * **DMA:** Utilizado en transmisión de audio y comandos SPI rápidos.

---

## 2. La Pantalla LCD (SH1106) y la Prevención de Corrupción

La pantalla de la radio es una LCD gráfica monocromática de **128x64 píxeles**, controlada por un controlador compatible con **SH1106** a través de una interfaz serial SPI de 4 hilos.

### El Buffer de Pantalla (`gFrameBuffer`)
El firmware mantiene una copia exacta de la pantalla en la memoria RAM (un buffer de $128 \times 8 = 1024$ bytes, ya que cada byte representa una columna vertical de 8 píxeles). Las funciones de dibujo escriben en RAM, y periódicamente una tarea de sistema vuelca este buffer al controlador físico de la pantalla.

### El Bug de Pantalla Partida o Corrupción
Originalmente, cuando la radio operaba en un modo personalizado (como el menú de Half-Life o la pantalla de telemetría), las tareas periódicas en segundo plano (como el monitoreo de frecuencia o actualizaciones de canal) llamaban a `DisplayHomePage()` o `DisplayRadioHome()`. Estas funciones repintaban el fondo del VFO (frecuencias, canal, medidor de señal) sobreescribiendo el buffer y corrompiendo la interfaz del usuario.

### La Solución Implementada
Blindamos el buffer de pantalla agregando una guarda de ejecución lógica al inicio de las funciones críticas de pintado en `src/Gui/DisplayMain.c`:
```c
if (g_sysRunPara.sysRunMode == MODE_HL_MENU ||
    g_sysRunPara.sysRunMode == MODE_DASHBOARD ||
    g_sysRunPara.sysRunMode == MODE_SLAVE_LISTEN ||
    g_sysRunPara.sysRunMode == MODE_MASTER_PAIR ||
    g_sysRunPara.sysRunMode == MODE_DTMF_ANI)
{
    return; // Interrumpe el pintado nativo de inmediato si hay una pantalla de Half-Life activa.
}
```
Esto garantiza inmunidad total contra parpadeos o sobreposiciones gráficas.

---

## 3. Escaneo del Teclado y Detección del Botón Lateral

El teclado frontal se organiza en una matriz física de **4 columnas × 4 filas**. Los botones PTT, SideKey1 (PTT-Superior) y SideKey2 (PTT-Inferior/Radio) están conectados a pines GPIO individuales.

### Flujo de Intercepción del Botón FM / SideKey2:
1. **KEY_ScanTask (cada 10ms):** El microcontrolador escanea los pines GPIO. Al pulsar el botón lateral inferior, la lectura de hardware reporta el código físico **`KEYID_SIDEKEY2`**.
2. **Traducción de Configuración:** Dependiendo de la función programada por el usuario en el menú de la radio (menú PF2), el firmware convierte el evento físico en un evento lógico (por ejemplo, **`KEYID_FM`** para encender la radio FM).
3. **El Punto de Inserción Perfecto:** Interceptamos el evento en la raíz lógica del sistema: la función `SideKey_Process(U8 realEvent)` dentro de `src/App/AppMain.c`. Al redirigir `case KEYID_FM:`, garantizamos que presionar el botón lateral inferior siempre lance el menú unificado de Half-Life, manteniendo la tecla `*` y las demás teclas del teclado frontal 100% nativas y libres para sus propósitos originales.

---

## 4. ¿Cómo Funciona la Escritura en la Flash (Bootloader)?

El KD32F328 tiene un cargador de arranque residente en una ROM interna no volátil. Este bootloader permite flashear el código a la memoria Flash del chip mediante comunicación serial UART (pines del jack de audio de la radio, Rx/Tx).

### Paso 1: Puesta en Modo Programación (PTT + ON)
El encendido de la radio con el botón **PTT presionado** obliga al microcontrolador a ejecutar una rutina de arranque especial en ROM. Se enciende el LED superior de la linterna de forma fija, lo que nos indica visualmente que el microcontrolador está esperando comandos UART a una velocidad de **38400 baudios**.

### Paso 2: El Protocolo de Autenticación (Handshake)
Para evitar que se escriba firmware corrupto o no autorizado, el cargador de arranque implementa una máquina de estados segura:
1. El script de flasheo (`k6_flash.py`) envía una ráfaga inicial de sincronización serial (bytes de saludo como `0x05`).
2. La radio responde confirmando su modelo y versión de hardware (`"K6"` o `"K5"`).
3. El script envía un paquete de datos específico de enlace (Handshake) conteniendo firmas criptográficas o de coincidencia. Si la firma es exitosa, el Bootloader desbloquea los registros de escritura de la memoria Flash del microcontrolador.

### Paso 3: Borrado y Escritura por Páginas (Flash Programming)
* La memoria Flash no puede reescribirse bit a bit; primero debe borrarse un bloque o sector completo y luego programarse.
* El script serial divide el binario `.bin` en bloques de **512 bytes** (tamaño de página física de la flash del KD32F328).
* Por cada bloque, se envía un comando serial de escritura (`0x19` / `0x1A`), el Bootloader borra el sector correspondiente en el mapa de memoria `0x08000000`, copia los 512 bytes recibidos en la SRAM y los escribe físicamente en las celdas de silicio de la Flash.
* El Bootloader verifica el checksum de cada bloque antes de confirmar de vuelta al host.
* Una vez completado el 100%, el script envía un comando de reinicio, provocando que la radio haga un reset físico por software y salte a ejecutar el nuevo código en `0x08000000`.

---

## 5. El Botón PTT y su Comportamiento Estándar Comercial

En radios comerciales, el botón **PTT (Push-to-Talk)** tiene prioridad absoluta sobre cualquier otra actividad de software (menús, agenda, telemetría).

* **Salida Instantánea:** Modificamos la función principal de parada de tareas del sistema **`ExitAllFunction()`** en [Functions.c](file:///home/meaxheeadroom/proyectos/BAOFENG-UV-K6-Firmware-halflife/src/Core/Functions.c) y la rutina de escaneo del PTT **`PTT_ScanTask()`** en [key_ptt.c](file:///home/meaxheeadroom/proyectos/BAOFENG-UV-K6-Firmware-halflife/src/Driver/key_ptt.c).
* En el momento exacto en que se presiona el botón PTT estando dentro del Menú Half-Life, la agenda de contactos DTMF, o el Dashboard de Telemetría:
  1. Se suspende de inmediato el renderizado del menú personalizado.
  2. `g_sysRunPara.sysRunMode` se restablece a `MODE_MAIN`.
  3. Se redibuja la pantalla de frecuencia de inicio limpia (`DisplayHomePage()`).
  4. Se inicializa la etapa de transmisión de RF (`Radio_EnterTxMode()`) en la frecuencia activa del canal de VFO seleccionado.
  5. Al soltar el PTT, la radio regresa de forma nativa a la escucha normal en la frecuencia, sin que queden rastros gráficos corruptos de la interfaz anterior.
