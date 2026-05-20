# 🧬 Saneamiento 1 — Auditoría Completa de Externs Inline
### Firmware BaoFeng UV-K6 / Half-Life Tactical Mod · Rama `half-life-advanced`

---

## 🔍 Diagnóstico: ¿Qué es un Extern Inline y Por Qué Rompe Todo?

En C, cuando un archivo `.c` necesita llamar a una función definida en otro archivo `.c`, debe conocer su firma (nombre, tipo de retorno, parámetros). El método **correcto** es declarar esa función en un archivo `.h` y hacer `#include` de ese `.h`.

El método **incorrecto** (y el que tiene este firmware) es escribir la declaración `extern` directamente dentro del cuerpo del archivo `.c`:

```c
// FORMA INCORRECTA (extern inline - el origen del caos)
void algunaFuncion(void)
{
    extern void DisplayHomePage(void);  // declarado aqui dentro
    DisplayHomePage();
}
```

### Por que esto rompe la logica del PTT y otras funciones?

1. **Duplicacion Silenciosa**: Si la firma de la funcion cambia en el futuro (por ejemplo, se le añade un parametro), el `extern` inline **no da error de compilacion inmediato** pero genera un comportamiento indefinido en tiempo de ejecucion. El MCU salta a una direccion incorrecta - reboot.
2. **Compilacion Parcial**: El compilador de ARM GCC resuelve cada unidad de compilacion (`.c`) por separado. Si el mismo `extern` esta declarado con tipos distintos en dos archivos, **no hay error** pero los tipos de los argumentos en el stack de llamadas se desalinean - corrupcion de punteros.
3. **El Problema del PTT**: `key_ptt.c` y `Functions.c` declaran `extern void DisplayHomePage(void)` y `extern void RxReset(void)` de forma inline. Si en algun momento el compilador resuelve estas unidades en ordenes distintos o con versiones distintas del encabezado en cache, las llamadas apuntan a direcciones desplazadas - regreso involuntario al menu VFO al presionar PTT.

---

## Inventario Completo de Externs Inline Encontrados

**Total de externs inline encontrados: 42 declaraciones en 7 archivos `.c`**

| N | Archivo | Externs Inline | Severidad |
|---|---------|---------------|-----------|
| 1 | `src/App/AppHalfLife.c` | 13 | CRITICO |
| 2 | `src/App/AppMain.c` | 10 | CRITICO |
| 3 | `src/Gui/DisplayMain.c` | 9 | ALTO |
| 4 | `src/Driver/key_ptt.c` | 3 | CRITICO |
| 5 | `src/Core/Functions.c` | 2 | MEDIO |
| 6 | `src/App/AppDtmf.c` | 1 | MEDIO |
| 7 | `src/Voice/Beep.c` | 1 | BAJO |

Nota: `src/syscalls.c` contiene 3 externs con `__attribute__((weak))` — estos son **correctos y necesarios** para el runtime del sistema. Se dejan intactos.

---

## CRITICO — `src/App/AppHalfLife.c` (13 externs inline)

Este es el archivo mas contaminado. Contiene 13 declaraciones de funciones que ya **existen en cabeceras correctas** pero que fueron re-declaradas inline por falta del `#include` adecuado.

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L52 | `extern void BatteryGetLevel(void)` | Battery.h |
| L172 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h (propio) |
| L263 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h (propio) |
| L348 | `extern void HL_BackgroundInactivityTask(void)` | NO EXISTE en ningun .h |
| L456 | `extern void Font_Read_8x16_ASCII(...)` | FlashFont.h |
| L556 | `extern U8 g_rfState` | Globe.h |
| L791 | `extern void Rfic_ConfigRxMode(void)` | DevFD6818.h |
| L792 | `extern void Rfic_SetScramble(U8, U32)` | **NO EXISTE en ningun .h** |
| L811 | `extern U8 g_rfState` | Globe.h (duplicado) |
| L868 | `extern void Rfic_ConfigRxMode(void)` | DevFD6818.h (duplicado) |
| L869 | `extern void Rfic_SetScramble(U8, U32)` | **NO EXISTE en ningun .h** |
| L932 | `extern void DtmfSendCodeOn(U8 type)` | AppDtmf.h |
| L987 | `extern void DisplayHomePage(void)` | DisplayMain.h |

**Plan de accion:**
1. Añadir `extern void Rfic_SetScramble(U8 group, U32 freq);` a `src/Driver/DevFD6818.h`.
2. Eliminar los 13 externs inline de `AppHalfLife.c`. El `#include "includes.h"` ya jalara todos los headers necesarios.

---

## CRITICO — `src/App/AppMain.c` (10 externs inline)

`AppMain.c` gestiona el procesamiento de teclas del menu principal. Aqui es donde se decide si el firmware entra al menu Half-Life o al VFO estandar. Los externs incorrectos aqui son la **causa raiz mas probable** del bug del PTT.

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L101 | `extern U8 g_hlMenuIndex` | AppHalfLife.h |
| L106 | `extern void UI_DisplayDashboard(void)` | AppHalfLife.h |
| L112 | `extern void MasterPairInit(void)` | AppHalfLife.h |
| L118 | `extern void UI_DisplaySlaveListen(void)` | AppHalfLife.h |
| L124 | `extern U8 g_aniContactIndex` | AppHalfLife.h |
| L126 | `extern void UI_DisplayAniContacts(void)` | AppHalfLife.h |
| L138 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h |
| L145 | `extern U8 g_hlMenuIndex` | AppHalfLife.h (duplicado) |
| L148 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h (duplicado) |
| L348 | `extern void Rfic_SetScramble(U8, U32)` | **NO EXISTE en ningun .h** |

**Plan de accion:**
1. Añadir `#include "AppHalfLife.h"` en `AppMain.c` (despues de `#include "includes.h"`).
2. Eliminar los 10 externs inline — todos estaran cubiertos por los headers.

---

## CRITICO — `src/Driver/key_ptt.c` (3 externs inline)

Este archivo es el que **maneja el boton fisico PTT**. Cualquier extern mal resuelto aqui causa directamente un comportamiento inesperado al presionar el boton.

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L27 | `extern void DisplayHomePage(void)` | DisplayMain.h |
| L28 | `extern void RxReset(void)` | RadioTask.h |
| L45 | `extern void MasterPairTrigger(void)` | AppHalfLife.h |

**Plan de accion:**
1. Añadir `#include "AppHalfLife.h"` en `key_ptt.c` — `AppHalfLife.h` ya incluye `includes.h` que trae `DisplayMain.h` y `RadioTask.h`.
2. Eliminar los 3 externs inline.

---

## ALTO — `src/Gui/DisplayMain.c` (9 externs inline)

`DisplayMain.c` es el motor de renderizado principal. Actualmente declara inline todos los iconos y funciones de Half-Life que necesita.

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L298 | `extern const U8 iconScr_Seed1[17]` | AppHalfLife.h |
| L299 | `extern const U8 iconScr_Seed2[17]` | AppHalfLife.h |
| L300 | `extern const U8 iconScr_Seed3[17]` | AppHalfLife.h |
| L301 | `extern const U8 iconScr_Seed4[17]` | AppHalfLife.h |
| L302 | `extern const U8 iconScr[17]` | **NO EXISTE — variable fantasma** |
| L555 | `extern void UI_DisplaySlaveListen(void)` | AppHalfLife.h |
| L561 | `extern void UI_DisplayMasterPair(void)` | AppHalfLife.h |
| L567 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h |
| L573 | `extern void UI_DisplayAniContacts(void)` | AppHalfLife.h |

ATENCION: `iconScr[17]` en L302 **no existe en ningun header ni archivo `.c`**. Es un extern fantasma que apunta a memoria no inicializada. Debe eliminarse.

**Plan de accion:**
1. Añadir `#include "AppHalfLife.h"` en `DisplayMain.c`.
2. Eliminar los 8 externs inline cubiertos. Eliminar el `iconScr` fantasma.

---

## MEDIO — `src/Core/Functions.c` (2 externs inline)

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L39 | `extern void DisplayHomePage(void)` | DisplayMain.h (via includes.h) |
| L40 | `extern void RxReset(void)` | RadioTask.h (via includes.h) |

**Plan de accion:** Ambas ya estan incluidas via `includes.h`. Eliminar los externs inline redundantes.

---

## MEDIO — `src/App/AppDtmf.c` (1 extern inline)

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L541 | `extern void HL_ProcessIncomingOTAP(const char *dtmfString)` | AppHalfLife.h |

**Plan de accion:** Añadir `#include "AppHalfLife.h"` en `AppDtmf.c` y eliminar el extern inline.

---

## BAJO — `src/Voice/Beep.c` (1 extern inline)

| Linea | Extern Inline | Ya existe en |
|-------|--------------|-------------|
| L4 | `extern STR_MENUINFO g_menuInfo` | AppMenu.h (via includes.h) |

**Plan de accion:** Eliminar el extern inline redundante.

---

## Hallazgos Adicionales: Funciones Sin Cabecera

Hay 2 funciones que no existen en ningun header pero son llamadas desde multiples archivos:

| Funcion | Definida en | Llamada desde | Accion |
|---------|-------------|--------------|--------|
| `Rfic_SetScramble(U8, U32)` | `DevFD6818.c` | `AppHalfLife.c`, `AppMain.c`, `DevFD6818.c` | Añadir a `DevFD6818.h` |
| `HL_BackgroundInactivityTask()` | No encontrada | `AppHalfLife.c` L348 | Funcion fantasma — Eliminar referencia |
| `iconScr[17]` | No encontrada | `DisplayMain.c` L302 | Variable fantasma — Eliminar |

---

## Plan de Ejecucion Secuencial

```
Paso 1: DevFD6818.h    → Añadir Rfic_SetScramble al header oficial
Paso 2: AppHalfLife.c  → Eliminar 13 externs inline
Paso 3: AppMain.c      → Añadir #include "AppHalfLife.h" + eliminar 10 externs
Paso 4: key_ptt.c      → Añadir #include "AppHalfLife.h" + eliminar 3 externs
Paso 5: Functions.c    → Eliminar 2 externs redundantes
Paso 6: AppDtmf.c      → Añadir #include "AppHalfLife.h" + eliminar 1 extern
Paso 7: DisplayMain.c  → Añadir #include "AppHalfLife.h" + eliminar 8 externs
Paso 8: Beep.c         → Eliminar 1 extern redundante
Paso 9: Compilar       → .venv/bin/pio run (cero implicit declaration warnings)
Paso 10: Git commit    → Sellar el saneamiento en rama half-life-advanced
```

---

## Criterios de Aceptacion

Al finalizar, la compilacion con PlatformIO debe producir:
- Cero warnings de tipo `implicit declaration of function`
- Cero warnings de tipo `conflicting types`
- [SUCCESS] con RAM menos de 40% y Flash menos de 48%
- Comportamiento de PTT verificado fisicamente en el radio
