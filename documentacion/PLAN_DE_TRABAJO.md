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
- [ ] **Etapa 2:** Driver de Teclado y Audio (Interacción básica).
- [ ] **Etapa 3:** Sistema de Menús minimalista (GUI/LCD).
- [ ] **Etapa 4:** Protocolo Half-Life (Telemetría/OTAP).

## 3. Registro de Cambios (Log de Integridad)
* *2026-05-21:* Completada Etapa 1. Inyección de script de arranque minimalista (startup.S), extracción e integración de dependencias GPIO/RCC/IWDG al src/Driver, limpieza de headers y parpadeo funcional del LED.
* *YYYY-MM-DD:* Integración de headers del driver KD32F328. Eliminación de dependencias circulares en includes.h.

## 4. Reglas para Agentes
1. `src/` es zona de escritura. Mantener el minimalismo (No añadir código muerto).
2. `src_sucio/` es zona de referencia (Solo lectura).
3. Todo cambio en `src/` debe ser auditable y compilar (`pio run`).
