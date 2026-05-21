# Roadmap: Proyecto Half-Life (Rama: minifirm)


Tienes tres tipos de documentos. Cada uno debe tener un rol claro para el agente:

PLAN_DE_TRABAJO.md (El Mapa): El estado actual, la hoja de ruta y las reglas de integración.

GUIA_INSTALACION_RAPIDA.md / EXPLICACION_TECNICA_K6A.md (El Manual del Hardware): Estos son críticos para que el agente entienda las limitaciones del chip BK4829 y los registros del KD32F328.

MANUAL_DE_USUARIO_HALFLIFE.md (La Visión): Esto le da al agente el "qué" estamos construyendo, no solo el "cómo". Es vital para que no tome decisiones de diseño que contradigan la lógica de tu protocolo.

## 1. Estado Actual
* **Kernel:** Bare-metal funcional, watchdog integrado, compilación exitosa (`pio run` = SUCCESS).
* **Arquitectura:** Doble capa (`src/` = limpio, `src_sucio/` = referencia).
* **Último hito:** Integración de headers de bajo nivel (IWDG, RCC, GPIO) y eliminación de bloatware.

## 2. Plan de Trabajo (Próximos Pasos)
- [x] **Etapa 1:** Implementar Blink Test (GPIO/LED).
- [x] **Etapa 2:** Driver de Teclado (Matriz y Botones Discretos PTT/SideKeys).
- [x] **Etapa 3:** Sistema de Gestión de Eventos (Máquina de Estados FSM).
- [ ] **Etapa 4:** Sistema de Menús minimalista (GUI/LCD).
- [ ] **Etapa 5:** Protocolo Half-Life (Telemetría/OTAP).

## 3. Registro de Cambios (Log de Integridad)
* *2026-05-21:* Auditoría de Integridad (Hardened Core). Inyectado modificador `volatile` en `g_uiState` y `g_keyScan` para prevenir optimizaciones del compilador. Reemplazados bucles `while` crudos en el driver SPI (`Sc5260.c`) por conteo con `timeout`. Eliminado `DelayMs(100)` bloqueante en `Protocol_VRFR_Transmit()`, trasladando el parpadeo del LED al planificador asíncrono (`App_100msTask`). Marcado `EnterResetMode` en la UI antigua como trampa peligrosa (`TRAP DANGER`).
* *2026-05-21:* Completada Etapa 3. Refactorización de AppRunTask con inyección de Máquina de Estados Finita (AppEventManager). Implementado flujo VRFR y debug visual.
* *2026-05-21:* Parche Crítico Etapa 2. Desvinculado el escaneo del teclado (KEY_ScanTask y ExtraKeys_ScanTask) del timer de hardware SysTick (g_10msFlag), implementando polling directo en el bucle principal y una máquina de estados de dos fases para el antirrebote del PTT (PA10).
* *2026-05-21:* Completada Etapa 2 y 2.1. Driver de teclado por polling (matriz principal) y botones discretos (PTT, SideKey1, SideKey2) desvinculando bloqueos del USART de fábrica.
* *2026-05-21:* Completada Etapa 1. Inyección de script de arranque minimalista (startup.S), extracción e integración de dependencias GPIO/RCC/IWDG al src/Driver, limpieza de headers y parpadeo funcional del LED.

## 4. Reglas para Agentes
1. `src/` es zona de escritura. Mantener el minimalismo (No añadir código muerto).
2. `src_sucio/` es zona de referencia (Solo lectura).
3. Todo cambio en `src/` debe ser auditable y compilar (`pio run`).
