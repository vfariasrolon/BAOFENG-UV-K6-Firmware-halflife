  .syntax unified
  .cpu cortex-m0
  .thumb

.global g_pfnVectors
.global Reset_Handler

/* Definición de la tabla de vectores en la sección esperada por el Linker */
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
g_pfnVectors:
  .word 0x20006000    /* Top of Stack (Stack Pointer) */
  .word Reset_Handler /* Reset Handler (Entry Point) */

/* Código de arranque */
  .section .text.Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr r0, =0x20006000 /* Aseguramos que el SP apunte correctamente */
  mov sp, r0
  bl main             /* Salto incondicional a la función main en C */

Infinite_Loop:
  b Infinite_Loop     /* Fallback por si main() retorna */
