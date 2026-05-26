# BaoFeng UV-K6 Firmware: Proyecto Half-Life

Este repositorio contiene el código fuente, herramientas de flasheo, scripts de parcheo y la documentación completa para el firmware personalizado **Half-Life** del radio **BaoFeng UV-K6** (basado en el microcontrolador KD32F328 y el módem BK4819/BK4829).

El objetivo de este proyecto es transformar la radio en un dispositivo táctico con un protocolo seguro de comunicaciones (VRFR) y con capacidades experimentales avanzadas de red (como un módem FSK).

## 📂 Estructura del Proyecto

El proyecto está organizado de la siguiente manera para mantener un entorno limpio y profesional:

### Código Fuente
* **`src/`**: Carpeta principal con el código fuente activo del firmware. Contiene aplicaciones, controladores, interfaces y librerías del sistema.
* **`src_base/`**: Código fuente original o antiguo ("sucio"), mantenido como referencia base.

### Documentación (`.md` y `.pdf`)
* **`documentacion/`**: Archivos clave de arquitectura y manuales de usuario.
  * `ARQUITECTURA_HALFLIFE.md`: Detalles de diseño de alto nivel.
  * `MANUAL_DE_USUARIO_HALFLIFE.md`: Guía de operación.
  * `EXPLICACION_TECNICA_K6A.md`: Explicación técnica profunda del equipo.
  * `PLAN_DE_TRABAJO.md`: Hoja de ruta del desarrollo.
  * `GUIA_INSTALACION_RAPIDA.md`: Cómo compilar y flashear el firmware.
  * `BK4819 guia de programador.pdf`: Manual de programación (filtrado) del chip de RF.
* **`docs/`**: Documentación más específica de hardware y subsistemas.
  * `AudioEngine_PWM_DDS.md`: Detalles del motor de audio.
  * `BK4829_ReverseEngineering.md`: Ingeniería inversa del chip de radio.
  * `CortexM0_Bootloader_Interrupts.md`: Funcionamiento de interrupciones del microcontrolador.
* **`research/`**: Notas de investigación, reportes técnicos e ideas de desarrollo (e.g. `REPORTE_TECNICO_HALF_LIFE.md`).

### Herramientas y Scripts (Python)
* **`tools/`**: Herramientas principales para interactuar con la radio de forma directa a través del puerto serie.
  * `k6_rescue.py`: Script principal para **flashear** el firmware al radio.
  * `k6_gui.py`: Herramienta con interfaz gráfica de usuario.
  * `k6_reg_editor.py`: Lector/escritor avanzado de registros del chip en tiempo real.
* **`scripts/`**: Colección de scripts de parcheo, corrección (`fix_*.py`) y utilidades (`patch_*.py`) usados durante el desarrollo e investigación.

### Archivos Generados, Logs y Recursos
* **`out/`**: Directorio donde se guardan los archivos binarios compilados listos para flashear (`.bin` y `.hex`).
* **`logs/`**: Salidas de terminal de puerto serie, volcados de memoria y registros de depuración (`salida*.txt`, `dumps.txt`).
* **`assets/`**: Archivos gráficos del proyecto (imágenes, logos de pantalla, etc.).

### Archivos de la Raíz
En la raíz encontrarás únicamente los archivos necesarios para preparar, configurar y compilar el entorno:
* **`platformio.ini` / `platform.json`**: Configuración de compilación para PlatformIO.
* **`Makefile`**: Para la compilación manual sin PlatformIO.
* **`kd32f328xb.ld`**: Script del enlazador (Linker script) crítico para el mapa de memoria del MCU.
* **`setup.sh` / `quick_run.sh`**: Scripts en bash para instalar dependencias y compilar rápidamente.
* **`.env.example`**: Plantilla para variables de entorno locales.
* **`requirements.txt`**: Librerías necesarias de Python (ej: `pyserial`).

## 🚀 Cómo compilar y subir

1. **Instalar dependencias**: Ejecutar `./setup.sh` o instalar desde `requirements.txt`.
2. **Compilar**: Usa el comando `pio run` (o `./venv/bin/pio run`). El archivo final se generará en la carpeta `out/`.
3. **Flashear**: Utiliza la herramienta de rescate conectando tu radio por cable serie.
   ```bash
   python tools/k6_rescue.py
   ```
