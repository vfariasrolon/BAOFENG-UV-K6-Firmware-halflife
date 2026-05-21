# Manual de Usuario: Firmware Half-Life (Baofeng UV-K6A / Quansheng K6)

Este manual te explica de forma clara y detallada cómo operar las nuevas funciones del firmware modificado en tu radio **Baofeng UV-K6A**.

---

## ⚡ 1. Acceso y Navegación del Menú Half-Life

Diseñamos una vía de acceso rápida de hardware para que no tengas que interferir con ninguno de los menús estándar de fábrica.

* **Abrir el Menú:** Presiona el botón físico **FM** (el botón lateral inferior, situado justo **debajo del PTT**). El menú principal `"MENU HALF-LIFE"` aparecerá de forma instantánea.
* **Cerrar el Menú:** Presiona de nuevo el botón **FM** (o presiona la tecla **EXIT**) para salir de inmediato y regresar a tu pantalla de frecuencias (VFO) de inicio.
* **Desplazamiento:** Utiliza las teclas físicas de flechas **▲ (Arriba)** y **▼ (Abajo)** del teclado frontal para mover la barra de selección.
* **Seleccionar / Entrar:** Presiona la tecla **MENU** en el teclado frontal para confirmar y entrar a la opción seleccionada.
* **Regresar / Cancelar:** Presiona la tecla **EXIT** para volver a la pantalla anterior.

---

## 📻 2. Estructura y Funciones del Menú

Al abrir el menú de Half-Life, tendrás acceso a las siguientes opciones personalizadas:

### 1. TELEMETRÍA (Dashboard de Monitoreo)
Esta pantalla te permite auditar el estado físico de los nodos esclavos remotos en tiempo real.
* **¿Qué muestra?**
  * Una lista dinámica de dispositivos esclavos activos en tu red.
  * El **voltaje de batería** reportado por cada nodo remoto.
  * La **fuerza de la señal (RSSI)** con la que estás recibiendo sus paquetes.
  * **Actualizaciones en tiempo real:** Si un esclavo transmite su reporte o se apaga (Timeout), la interfaz gráfica se actualiza y redibuja de inmediato sin necesidad de salir.

### 2. MDO MAESTRO (Master Mode)
Configura tu radio para que actúe como la estación de control central del enlace.
* Se utiliza para enviar señales automáticas de sincronización (OTAP), gestionar las transmisiones de datos y registrar las radios satelitales que ingresen al perímetro.

### 3. MDO ESCLAVO (Slave Mode)
Configura tu radio para operar como un nodo remoto pasivo.
* En este modo, la radio transmite de forma automática y silenciosa pequeños paquetes de datos con su telemetría (estado de batería, actividad) hacia la estación base (Maestro) a intervalos regulares.

### 4. AGENDA DTMF / ANI (Listado de Contactos Rápidos)
Una libreta de direcciones digital optimizada para llamadas y señalización DTMF/ANI.
* **Tu ID Propio:** En la parte superior de la pantalla verás tu propio **ID DTMF de transmisión (ANI)** para fácil referencia.
* **Lista de Contactos:** Una rejilla scrollable con hasta **20 contactos preprogramados** mostrando sus nombres y códigos DTMF individuales.
* **Marcación Instantánea:** Desplázate con las flechas **▲** / **▼** hasta el contacto deseado y presiona **MENU**. La radio activará automáticamente el PTT y transmitirá la secuencia de tonos DTMF correspondiente de forma súper rápida.

---

## 🔒 3. Prioridad Absoluta del PTT (Seguridad Comercial)

Para garantizar que nunca te quedes incomunicado en una emergencia mientras navegas por los menús:
* **Interrupción al Hablar:** Si estás en cualquier pantalla especial (Menú Half-Life, Agenda DTMF, Telemetría, etc.) y necesitas hablar, simplemente **mantén presionado el botón PTT**.
* **Acción Inmediata:** La radio cerrará automáticamente cualquier menú personalizado, regresará al modo de frecuencia principal (**MODE_MAIN**), limpiará la pantalla y entrará en transmisión (TX) en tu frecuencia del canal VFO seleccionado.
* **Al soltar el PTT:** Te quedarás directamente en el modo frecuencia listo para escuchar a tu interlocutor, garantizando una operación fluida y libre de bugs visuales.

---

## 💎 4. Indicador Especial de Encriptación: Los Dos Rombos `♦♦`

Cuando estés escuchando frecuencias en la pantalla principal (VFO A o B), presta atención a los iconos debajo del texto de las frecuencias:
* **El Icono `♦♦`:** Si ves dos rombos sólidos alineados debajo de los decimales de la frecuencia, significa que la **Secrafonía / Scrambler (Encriptación de Voz)** por hardware está **activada** utilizando la clave militar personalizada de Half-Life.
* **Función:** Tu voz será distorsionada digitalmente antes de enviarse al aire. Solo los radios que compartan tu firmware y tengan activa la misma semilla del secrafonador podrán descifrar y escuchar tu voz con claridad; para cualquier otra radio o escáner, se escuchará como ruido inteligible.
