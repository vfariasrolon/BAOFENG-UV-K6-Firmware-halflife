# Guía de Instalación y Replicación: Baofeng UV-K6 Half-Life en Ubuntu / Linux

Esta guía está diseñada para que puedas replicar exactamente tu entorno de desarrollo y flasheo en **cualquier otra computadora con Ubuntu o Linux** en menos de 5 minutos, de forma 100% automatizada y libre de errores.

---

## 🚀 1. Configuración Súper Rápida (El Script Automatizado)

Para simplificar todo el proceso, creamos un instalador maestro (`setup.sh`). Solo debes abrir tu terminal en la carpeta del repositorio y ejecutar:

```bash
chmod +x setup.sh
./setup.sh
```

### ¿Qué hace este script de forma automática por ti?
1. Actualiza el sistema e instala las herramientas esenciales de desarrollo (`build-essential`, `python3-venv`, `git`).
2. Crea el entorno virtual de Python en `./venv`.
3. Instala **PlatformIO** y **PySerial** dentro del entorno virtual (`requirements.txt`).
4. **Agrega tu usuario al grupo `dialout`** para resolver permanentemente el problema de permisos de USB.

---

## 🔌 2. Entendiendo y Resolviendo los Permisos de USB en Ubuntu

El mayor obstáculo en sistemas Ubuntu/Linux recién instalados es el acceso al cable de programación USB (`/dev/ttyUSB0`). Por defecto, el sistema bloquea el acceso de escritura a usuarios normales. Aquí te explicamos cómo se diagnostica y soluciona:

### A. Diagnóstico del Dispositivo
Para verificar si la computadora reconoce tu cable de programación y qué puerto le asignó:
1. **Ver el puerto asignado:**
   ```bash
   ls -l /dev/ttyUSB*
   ```
2. **Revisar registros de conexión del kernel (dmesg):**
   Si no aparece nada con el comando anterior, revisa si el chip del cable (CH340, CP2102, PL2303) fue detectado al conectarlo:
   ```bash
   sudo dmesg | grep -i usb
   # o bien:
   sudo dmesg | tail -n 20
   ```

### B. Métodos de Solución (Permisos)

#### Método 1: Agregar tu usuario al grupo `dialout` (Recomendado y Permanente)
El puerto `/dev/ttyUSB0` pertenece al grupo `dialout`. Si agregas tu usuario a este grupo, tendrás acceso total de por vida sin necesidad de usar `sudo`.
```bash
sudo usermod -aG dialout $USER
```
> [!IMPORTANT]
> **Paso Crítico:** Para que Linux aplique este cambio de grupo, **debes cerrar sesión en tu cuenta de Ubuntu y volver a iniciarla**, o bien escribir en tu terminal actual:
> ```bash
> newgrp dialout
> ```

#### Método 2: Cambiar permisos manualmente (Solución Temporal Rápida)
Si necesitas una solución rápida sin reiniciar o cerrar sesión, puedes abrir el puerto para cualquier usuario con:
```bash
sudo chmod 666 /dev/ttyUSB0
```
> [!WARNING]
> **Contra:** Este método es **temporal**. Cada vez que desconectes el cable USB o reinicies la computadora, Linux volverá a crear el puerto con sus permisos bloqueados originales, y tendrás que volver a escribir el comando. Por ello, el **Método 1** es infinitamente superior.

---

## 🛠️ 3. Compilación y Despliegue de Firmware

Una vez completado el script de instalación y aplicados los permisos, puedes compilar y flashear con estos comandos sencillos:

### Compilar el Firmware:
```bash
./venv/bin/pio run
```
*Este comando descarga las herramientas de compilación del microcontrolador DP32G030, compila el código y deposita el archivo `.bin` en la carpeta `out/`.*

### Flashear la Radio (Despliegue):
1. Enciende tu radio manteniendo presionado el botón **PTT** (la linterna superior se encenderá fija o el LED parpadeará en modo bootloader / UPDATE).
2. Conecta el cable de programación directo al PC (evita Hubs USB para evitar latencia de comunicación).
3. Ejecuta la herramienta de rescate interactiva con privilegios `sudo` para asegurar el control exclusivo del puerto serial:
   ```bash
   sudo ./venv/bin/python3 k6_rescue.py
   ```
   *El script detectará automáticamente tu puerto `/dev/ttyUSB0` (o el correspondiente), escaneará la carpeta `out/` y te presentará un menú interactivo para flashear el firmware custom de manera 100% segura.*

---

## ⚙️ 4. Uso del Archivo `.env` (Pros y Contras)

Hemos agregado compatibilidad nativa en el script `k6_flash.py` para leer variables de entorno como el puerto serial a través del archivo `.env`.

Para usarlo, haz una copia de la plantilla:
```bash
cp .env.example .env
```
Y edita el archivo `.env` configurando tu puerto personalizado (por ejemplo, `PORT=/dev/ttyUSB1`).

### ⚖️ Tabla de Pros y Contras de Usar un `.env` en este Proyecto:

| PROS 🟢 | CONTRAS 🔴 |
| :--- | :--- |
| **Configuración por Máquina:** Cada computadora puede tener su propio puerto serial asignado (ej. `ttyUSB0` en una PC, `ttyUSB1` en otra) sin modificar el código fuente. | **Complejidad Adicional:** Exige crear y mantener un archivo extra (`.env`) que los programadores novatos pueden olvidar configurar. |
| **Seguridad de Git:** El archivo `.env` se añade al `.gitignore` para evitar subir rutas o configuraciones privadas de hardware a GitHub. | **Soporte de Consola Directo:** A veces es más rápido pasar el puerto como argumento directo en consola (`python3 k6_flash.py /dev/ttyUSB1`) que editar un archivo de texto. |
| **Despliegues en la Nube / CI-CD:** Permite integrar la compilación y pruebas en sistemas automatizados usando variables estándar del sistema. | **Redundancia:** Para un proyecto sencillo de firmware, a veces resulta más limpio confiar en valores por defecto inteligentes (`/dev/ttyUSB0`). |

### 💡 Nuestra Solución Híbrida Inteligente:
Para darte **lo mejor de ambos mundos**, programamos `k6_flash.py` para que funcione con **jerarquía de prioridades**:
1. **Prioridad 1 (Máxima):** El argumento que le pases directamente en consola (ej. `python3 k6_flash.py /dev/ttyUSB2`).
2. **Prioridad 2 (Media):** La variable definida en tu entorno o en tu archivo `.env` (ej. `PORT=/dev/ttyUSB1`).
3. **Prioridad 3 (Mínima / Default):** El puerto estándar `/dev/ttyUSB0` si no se especifica nada más.

¡Esto te asegura compatibilidad absoluta sin obligarte a configurar nada si usas el puerto estándar!

---

## 🖥️ 5. Interfaz Gráfica de Usuario (Panel de Control de Escritorio)

Hemos diseñado una interfaz gráfica premium y multiplataforma usando **CustomTkinter**. Esta app es 100% compatible con **Windows, macOS (Intel y Apple Silicon M1/M2/M3) y Ubuntu**.

### ¿Cómo abrir la Interfaz Gráfica?
Desde la carpeta raíz del proyecto, simplemente ejecuta:
```bash
./venv/bin/python3 k6_gui.py
```

### Características de la Aplicación Gráfica:
1. **Detección Automática de Puertos:** Escanea dinámicamente tus puertos USB activos y los expone en un menú desplegable.
2. **Sondeo de Conexión (A/B Test):** Envía ráfagas de saludo a velocidades de 115200 (bootloader) y 9600 (programación) usando los comandos de estilo A y B para confirmar si la radio está escuchando antes de cualquier operación.
3. **Grabador Integrado (Flash):** Busca por defecto el firmware más nuevo generado por PlatformIO y te permite grabarlo en la radio con una hermosa barra de progreso real-time.
4. **Respaldo de EEPROM (Backup):** Descarga toda la configuración, calibración y canales de tu radio (8KB) en modo de programación nativo utilizando el protocolo ofuscado original con suma de comprobación XMODEM y la guarda de forma segura en tu disco duro.
5. **Logs Avanzados:** Muestra una consola de terminal interna integrada que te reporta detalladamente el flujo de bytes en tiempo real sin congelar la ventana.

