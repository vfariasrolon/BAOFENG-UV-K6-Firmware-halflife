# Arquitectura KD32F328 (Cortex-M0): Solución a los HardFaults del Bootloader

## El Problema: "El Bucle de la Muerte" y la Pantalla Blanca

Al desarrollar firmware customizado para el BaoFeng UV-K6 (KD32F328CBT6), se detectó un fallo crítico de arquitectura: en el momento exacto en que se llamaba a `__enable_irq()` para habilitar las interrupciones globales (por ejemplo, para arrancar el `TIM3` o el `PWM`), la radio colapsaba, mostrando una pantalla en blanco y reiniciándose en bucle infinito por acción del Hardware Watchdog.

## Análisis de Causa Raíz (Root Cause Analysis)

1. **Ausencia de VTOR en Cortex-M0:**
   El KD32F328 está basado en el núcleo ARM Cortex-M0 (esencialmente un clon del STM32F030). A diferencia de los Cortex-M3 o M4, el Cortex-M0 **carece del registro VTOR** (Vector Table Offset Register).
   Esto significa que el hardware de la CPU **siempre** lee la tabla de vectores de interrupción desde la dirección de memoria física `0x00000000`.

2. **Conflicto con el Bootloader de BaoFeng:**
   El bootloader de fábrica reside entre las direcciones `0x00000000` y `0x00001FFF`. Nuestro firmware customizado se enlaza para empezar a ejecutarse a partir del offset `0x08002000` (dejando intacto el bootloader).
   
   El problema ocurre cuando se dispara una interrupción de hardware (ej. `TIM3`). El hardware de la CPU salta a leer qué código debe ejecutar... pero como no tiene VTOR, busca en `0x00000000`. ¡Ahí encuentra los vectores del bootloader, no los nuestros!
   Al ejecutar el vector del bootloader para una interrupción que el bootloader no sabe manejar, el sistema causa una excepción de hardware no controlada (HardFault) o cae en un bucle infinito de reseteo.

## La Solución: Relocalización en SRAM y Remapeo SYSCFG

Para resolver este problema de hardware de forma permanente sin tocar el bootloader, se debe aplicar una técnica de relocalización dinámica usando la memoria RAM (SRAM) y el puente (bridge) `SYSCFG` del KD32F328.

### Paso 1: Reserva de Memoria en Linker Script (`kd32f328xb.ld`)
Se debe reservar espacio **estrictamente al inicio de la RAM** (`0x20000000`) para que las variables globales del programa no sean sobrescritas al copiar la tabla de vectores.

Se añade un offset de `0x100` (256 bytes) al inicio de la sección `.data`:

```ld
  /* Initialized data sections into "RAM" Ram type memory */
  .data :
  {
    . = ALIGN(4);
    . = . + 0x100; /* RESERVE 256 BYTES FOR VECTOR TABLE RELOCATION */
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
```

### Paso 2: Relocalización y Remapeo en `main.c`
Antes de limpiar la basura del bootloader y habilitar las interrupciones (`__enable_irq();`), se debe ejecutar la siguiente rutina:

1. **Copiar** los primeros 192 bytes (48 vectores) desde nuestra ROM (`0x08002000`) a la SRAM recién reservada (`0x20000000`).
2. **Remapear** (puentear por hardware) la dirección `0x00000000` para que apunte directamente a SRAM, usando el registro `CFGR1` del periférico `SYSCFG`.

```c
    // ---------------------------------------------------------
    // RELOCACIÓN DE VECTOR TABLE (CRÍTICO PARA CORTEX-M0)
    // 1. Copiamos la tabla desde Flash (0x08002000) a SRAM (0x20000000)
    uint32_t *vectors_flash = (uint32_t *)0x08002000;
    uint32_t *vectors_sram  = (uint32_t *)0x20000000;
    for (int i = 0; i < 48; i++) {
        vectors_sram[i] = vectors_flash[i];
    }
    
    // 2. Mapeamos la SRAM a la dirección 0x00000000 para que la CPU la use
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitar reloj SYSCFG
    SYSCFG->CFGR1 |= 0x03;                // SYSCFG_MemoryRemap_SRAM (0x03)
    // ---------------------------------------------------------
    
    // Purgar basura del bootloader
    SysTick->CTRL = 0; 
    SCB->ICSR = (1 << 25);
    NVIC->ICPR[0] = 0xFFFFFFFF;
    
    // Habilitar interrupciones globales de forma segura
    __enable_irq();
```

## Conclusión
Implementar este remapeo de SRAM puentea de forma segura el bootloader de fábrica en el momento que nuestro firmware arranca, permitiendo que el KD32F328 procese los vectores correctos. Esto estabiliza inmediatamente el sistema y permite usar funciones avanzadas que dependen de temporizadores (TimeManager) y Modulación por Ancho de Pulsos (AudioEngine) sin causar cuelgues del Watchdog.
