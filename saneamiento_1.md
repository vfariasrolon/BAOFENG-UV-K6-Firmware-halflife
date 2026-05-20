# Saneamiento 1 — Auditoria NASA: Firmware BaoFeng UV-K6 Half-Life
### Rama `half-life-advanced` | Ingeniero de referencia: NASA Embedded Systems Standard NPR 7150.2

---

## RESUMEN EJECUTIVO

El firmware actual es funcionalmente util pero **arquitectonicamente fragil**. Bajo estandares de sistemas criticos (NASA NPR 7150.2 / MISRA-C:2012), presenta **4 categorias de riesgo que comprometen la predictibilidad y tolerancia a fallos**:

| Categoria | Hallazgos | Riesgo |
|-----------|-----------|--------|
| Externs inline sin cabecera | 42 declaraciones en 7 archivos | CRITICO — desalineacion de tipos en ARM stack |
| Variables/funciones fantasma | 3 referencias a simbolos inexistentes | CRITICO — RAM no inicializada / salto a 0x00000000 |
| Estado global sin propietario unico | `g_sysRunPara.sysRunMode` escrito desde 14 archivos | ALTO — condicion de carrera en SysTick |
| Ausencia total de Watchdog (WDT) | IWDG nunca inicializado ni alimentado | CRITICO — bloqueo silencioso sin recuperacion |

---

## PARTE 1: EXTERNS INLINE — El Origen del Caos

### Por que un extern inline rompe el PTT?

En ARM Cortex-M3, el compilador GCC resuelve cada unidad de compilacion `.c` de forma **independiente**. Cuando un archivo declara:

```c
// INCORRECTO - dentro del .c
extern void DisplayHomePage(void);
DisplayHomePage();
```

El compilador confía ciegamente en esa declaracion local. Si en otro archivo la misma funcion tiene una firma diferente o si el linker la resuelve en un orden distinto, **los argumentos en el stack de la llamada se desalinean**. En un Cortex-M3 esto produce:
- Salto a direccion incorrecta (PC corruption)
- Retorno desde funcion a direccion basura (LR corruption)
- HardFault → `NVIC_SystemReset()` → el radio regresa al menu VFO

**Este es exactamente el comportamiento que reportas: PTT en Dashboard → menu principal.**

---

### Inventario Completo (42 externs inline en 7 archivos)

**CRITICO — `src/App/AppHalfLife.c` (13)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L52 | `extern void BatteryGetLevel(void)` | Battery.h |
| L172 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h (auto) |
| L263 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h (duplicado) |
| L348 | `extern void HL_BackgroundInactivityTask(void)` | **NO EXISTE — FANTASMA** |
| L456 | `extern void Font_Read_8x16_ASCII(...)` | FlashFont.h |
| L556 | `extern U8 g_rfState` | Globe.h |
| L791 | `extern void Rfic_ConfigRxMode(void)` | DevFD6818.h |
| L792 | `extern void Rfic_SetScramble(U8, U32)` | **NO EXISTE en ningun .h** |
| L811 | `extern U8 g_rfState` | Globe.h (duplicado) |
| L868 | `extern void Rfic_ConfigRxMode(void)` | DevFD6818.h (duplicado) |
| L869 | `extern void Rfic_SetScramble(U8, U32)` | **NO EXISTE en ningun .h** |
| L932 | `extern void DtmfSendCodeOn(U8 type)` | AppDtmf.h |
| L987 | `extern void DisplayHomePage(void)` | DisplayMain.h |

**CRITICO — `src/App/AppMain.c` (10)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
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

**CRITICO — `src/Driver/key_ptt.c` (3)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L27 | `extern void DisplayHomePage(void)` | DisplayMain.h |
| L28 | `extern void RxReset(void)` | RadioTask.h |
| L45 | `extern void MasterPairTrigger(void)` | AppHalfLife.h |

**ALTO — `src/Gui/DisplayMain.c` (9)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L298 | `extern const U8 iconScr_Seed1[17]` | AppHalfLife.h |
| L299 | `extern const U8 iconScr_Seed2[17]` | AppHalfLife.h |
| L300 | `extern const U8 iconScr_Seed3[17]` | AppHalfLife.h |
| L301 | `extern const U8 iconScr_Seed4[17]` | AppHalfLife.h |
| L302 | `extern const U8 iconScr[17]` | **NO EXISTE — VARIABLE FANTASMA** |
| L555 | `extern void UI_DisplaySlaveListen(void)` | AppHalfLife.h |
| L561 | `extern void UI_DisplayMasterPair(void)` | AppHalfLife.h |
| L567 | `extern void UI_DisplayHlMenu(void)` | AppHalfLife.h |
| L573 | `extern void UI_DisplayAniContacts(void)` | AppHalfLife.h |

**MEDIO — `src/Core/Functions.c` (2)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L39 | `extern void DisplayHomePage(void)` | DisplayMain.h via includes.h |
| L40 | `extern void RxReset(void)` | RadioTask.h via includes.h |

**MEDIO — `src/App/AppDtmf.c` (1)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L541 | `extern void HL_ProcessIncomingOTAP(const char*)` | AppHalfLife.h |

**BAJO — `src/Voice/Beep.c` (1)**

| Linea | Extern Inline | Cabecera correcta |
|-------|--------------|------------------|
| L4 | `extern STR_MENUINFO g_menuInfo` | AppMenu.h via includes.h |

---

## PARTE 2: SIMBOLOS FANTASMA — Riesgo de Ejecucion en Memoria No Mapeada

Tres referencias en el codigo apuntan a simbolos que **no existen en ningun archivo del proyecto**:

| Simbolo | Referenciado en | Tipo de fallo |
|---------|----------------|---------------|
| `HL_BackgroundInactivityTask()` | AppHalfLife.c L348 | El linker puede resolverlo a 0x00000000 — ejecucion en vector table |
| `iconScr[17]` | DisplayMain.c L302 | Array extern sin definir — lecturas de RAM aleatoria → pantalla corrupta |
| `Rfic_SetScramble(U8, U32)` | AppHalfLife.c, AppMain.c, DevFD6818.c | Funcion real en DevFD6818.c pero SIN declaracion en ningun .h — tipos sin garantia |

**Accion inmediata:**
- `HL_BackgroundInactivityTask` → Eliminar la referencia (la funcion no existe)
- `iconScr[17]` → Eliminar la declaracion extern fantasma en DisplayMain.c
- `Rfic_SetScramble` → Añadir `extern void Rfic_SetScramble(U8 group, U32 freq);` a `DevFD6818.h`

---

## PARTE 3: ESTADO GLOBAL SIN PROPIETARIO UNICO

`g_sysRunPara.sysRunMode` es la variable de maquina de estado del sistema. Actualmente es **mutada directamente desde 14 archivos distintos**:

```
16 escrituras: AppHalfLife.c
12 escrituras: AppMain.c
11 escrituras: Battery.c          <- Por que Battery escribe el modo?
 7 escrituras: AppFm.c
 5 escrituras: AppTask.c
 5 escrituras: AppDtmf.c
 4 escrituras: AppMenu.c
 3 escrituras: AppWeather.c
 ...
```

**Esto viola el principio de unico punto de control de estado (Single State Authority).** Cualquier archivo puede cambiar el modo del sistema en cualquier momento, incluyendo desde interrupciones, lo que hace imposible razonar sobre el estado del sistema.

**Propuesta NASA:** Implementar una funcion centralizada de transicion de estado:
```c
// En AppHalfLife.h
void HL_SetMode(U8 newMode);   // El UNICO punto de escritura de sysRunMode
U8   HL_GetMode(void);         // Lectura segura del estado actual
```

---

## PARTE 4: AUSENCIA TOTAL DE WATCHDOG (WDT) — FALLO CRITICO

**Hallazgo:** El KD32F328CBT6 tiene dos watchdogs de hardware (IWDG y WWDG). **Ninguno esta inicializado ni alimentado en todo el codigo.**

```c
// main.c — bucle principal
while(1)
{
    if(g_10msFlag) App_10msTask();   // No hay IWDG_feed() aqui
    if(g_50msFlag) App_50msTask();
    if(g_100msFlag) App_100msTask();
    if(g_500msFlag) App_500msTask();
    AppRunTask();
    AlarmTask();
}
```

**Consecuencia:** Si el sistema entra en un bucle infinito, bloquea en `DelaySysMs()`, o un puntero corrupto lleva la ejecucion a codigo basura, **el MCU se queda colgado indefinidamente sin posibilidad de auto-recuperacion**.

**Propuesta — IWDG de 1 segundo:**
```c
// En Board_Init() — inicializacion
IWDG->KR  = 0x5555;  // Enable write access
IWDG->PR  = 0x06;    // Prescaler /256 -> clock ~156Hz
IWDG->RLR = 156;     // Reload = 1 segundo de timeout
IWDG->KR  = 0xAAAA;  // Reload counter
IWDG->KR  = 0xCCCC;  // Start IWDG

// En App_10msTask() — alimentar el perro cada 10ms
IWDG->KR = 0xAAAA;   // Feed watchdog
```

---

## PARTE 5: HARDFAULT SIN TELEMETRIA — FALLO SILENCIOSO

El HardFault actual hace un reset inmediato sin registrar ningun dato:

```c
// ACTUAL — version ciega
void HardFault_Handler(void)
{
    NVIC_SystemReset();   // Reset inmediato, sin registro
}
```

En sistemas criticos esto es inadmisible porque impide diagnosticar la causa raiz. El MCU tiene un registro de estado de fallo (`SCB->CFSR`, `SCB->MMFAR`, `SCB->BFAR`) que guarda exactamente que fallo y en que direccion.

**Propuesta — HardFault con registro en SRAM:**
```c
// Estructura persistente en RAM (sobrevive el reset)
typedef struct {
    uint32_t cfsr;    // Tipo de fallo
    uint32_t hfsr;    // HardFault status
    uint32_t mmfar;   // Direccion de acceso a memoria invalida
    uint32_t bfar;    // Direccion de bus fault
    uint32_t pc;      // Program Counter donde ocurrio el fallo
    uint32_t lr;      // Link Register (quien llamo la funcion que fallo)
    uint32_t count;   // Contador de reinicios por fallo
} HardFaultLog_t;

// Colocar en seccion .noinit para que sobreviva el reset
static HardFaultLog_t g_faultLog __attribute__((section(".noinit")));

void HardFault_Handler(void)
{
    g_faultLog.cfsr  = SCB->CFSR;
    g_faultLog.hfsr  = SCB->HFSR;
    g_faultLog.mmfar = SCB->MMFAR;
    g_faultLog.bfar  = SCB->BFAR;
    g_faultLog.count++;
    NVIC_SystemReset();
}
```

---

## PARTE 6: DEFINICIONES `extern` EN ARCHIVOS `.c` — El Doble Anti-Patron

El firmware tiene **293 funciones definidas con la palabra clave `extern` en el .c**:

```c
// INCORRECTO
extern void RF_TxTask(void)    // <- extern en la definicion
{
    ...
}
```

`extern` en la **definicion** de una funcion en C es **ignorado silenciosamente por el compilador** (es equivalente a no ponerlo). Sin embargo, **induce a confusion** y puede ocultar casos donde el mismo simbolo esta definido en multiples unidades de compilacion sin error. En sistemas criticos esto se considera un defecto de Clase B (MISRA-C Rule 8.5).

**Plan:** Eliminar el `extern` de todas las definiciones de funciones en archivos `.c`. Las declaraciones en `.h` mantienen el `extern`.

---

## PLAN DE EJECUCION SECUENCIAL

```
FASE A — Sellar el header maestro (sin riesgo de regresion)
  Paso 1: DevFD6818.h  → Añadir Rfic_SetScramble al header oficial
  Paso 2: AppHalfLife.h → Verificar que todos los simbolos HL_ esten declarados

FASE B — Limpieza de externs inline (orden de dependencia)
  Paso 3: AppHalfLife.c  → Eliminar 13 externs inline + quitar extern de definiciones
  Paso 4: AppMain.c      → Añadir #include "AppHalfLife.h" + eliminar 10 externs
  Paso 5: key_ptt.c      → Añadir #include "AppHalfLife.h" + eliminar 3 externs
  Paso 6: Functions.c    → Eliminar 2 externs redundantes
  Paso 7: AppDtmf.c      → Añadir #include "AppHalfLife.h" + eliminar 1 extern
  Paso 8: DisplayMain.c  → Añadir #include "AppHalfLife.h" + eliminar 8 externs

FASE C — Tolerancia a Fallos de Hardware
  Paso 9:  Board_Init()     → Implementar IWDG con timeout de 1 segundo
  Paso 10: App_10msTask()   → Alimentar IWDG cada 10ms
  Paso 11: kd32f328_it.c   → Implementar HardFault con registro en SRAM .noinit

FASE D — Control de Estado Unico
  Paso 12: AppHalfLife.h   → Declarar HL_SetMode(U8) y HL_GetMode(void)
  Paso 13: AppHalfLife.c   → Implementar las funciones con validacion de rango
  Paso 14: Todos los .c    → Reemplazar escrituras directas por HL_SetMode()

FASE E — Verificacion
  Paso 15: .venv/bin/pio run
           → Cero warnings implicit declaration
           → Cero warnings conflicting types
           → [SUCCESS] RAM < 40%, Flash < 48%
  Paso 16: git commit —> Sellar saneamiento completo
```

---

## CRITERIOS DE ACEPTACION (NASA Go/No-Go)

| Criterio | Metodo de Verificacion | Umbral |
|----------|----------------------|--------|
| Cero externs inline en .c | `grep -rn 'extern ' src/ --include=*.c | grep ';'` | 0 resultados (exc. syscalls.c) |
| Cero simbolos fantasma | Compilacion sin `undefined reference` | 0 errores de linker |
| IWDG activo | Osciloscopia en NRST al desconectar SysTick | Reset < 1.1 segundos |
| HardFault con log | Forzar null dereference, verificar g_faultLog.count > 0 | count >= 1 |
| Estado centralizado | `grep -rn 'sysRunMode =' src/` | Solo HL_SetMode() |
| Build limpio | `.venv/bin/pio run` | [SUCCESS] sin warnings |
