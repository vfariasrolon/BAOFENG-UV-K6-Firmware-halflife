# Arquitectura del Sistema: Half-Life Kernel (Rama: minifirm)

## 1. Filosofía de Diseño
* **Minimalismo Táctico:** Solo se compila el código estrictamente necesario para la misión.
* **Separación de Capas:**
    - `src/`: Código fuente propio y drivers integrados.
    - `src_sucio/`: Referencia de ingeniería inversa (Solo lectura).
* **Entrada de Control:** El `main.c` es el orquestador principal.

## 2. Mapa de Directorios (Contexto para Agentes)
- `src/App/`: Lógica de alto nivel (Tasks, main, AppTask).
- `src/Common/`: Núcleo del sistema (startup.S, stubs, prototipos, tipos).
- `src/Driver/`: Drivers de hardware (GPIO, RCC, IWDG, LED).
- `src/Driver/inc/`: Headers de bajo nivel (mapeo de registros).

## 3. Flujo de Ejecución (Boot Process)
1. `startup.S`: Inicializa el Stack Pointer y salta a `main()`.
2. `main()`: Llama a `LED_Init()`, configura el `RCC`, y entra al `while(1)`.
3. `AppTask.c`: Ejecuta tareas programadas (ej. `IWDG_ReloadCounter`).

## 4. Reglas de Integración (Prompt para Agentes)
* Prohibido añadir dependencias que requieran archivos fuera de `src/` o `src/Driver/inc/`.
* Todo driver nuevo debe tener su implementación en `.c` y su prototipo en `prototypes.h`.
* Siempre verificar `pio run` antes de considerar una tarea como completada.
