# Motor de Audio Táctico (AudioEngine) - PWM & Direct Digital Synthesis (DDS)

## Descripción General
El `AudioEngine` es el módulo encargado de generar todos los sonidos y alertas audibles del sistema (ej. *Nextel Chirp*, tonos tácticos, alertas de batería) de forma digital, sin utilizar recursos bloqueantes (como retardos por `delay`). Utiliza la técnica **DDS (Direct Digital Synthesis)** a través de una onda PWM (Modulación por Ancho de Pulsos) filtrada en el hardware para reproducir formas de onda sinusoidales puras.

## Mapeo de Hardware (BaoFeng UV-K6)
Para generar audio, el firmware debe interactuar con dos pines críticos del microcontrolador KD32F328:

1. **PA11 (TIM1_CH4): Señal de Audio PWM**
   - **Función:** Genera la señal portadora PWM a 16 kHz.
   - **Modo:** `Alternate Function 2` (AF2) conectado internamente al canal 4 del Timer 1.
2. **PB2 (Speaker Enable): Control del Amplificador**
   - **Función:** Enciende y apaga físicamente el amplificador operacional que alimenta la bocina del radio.
   - **Modo:** Salida estándar Push-Pull (`GPIO_Mode_OUT`). `High` enciende la bocina, `Low` la silencia y ahorra energía.

## Teoría de Operación (DDS)

El sistema evita los pitidos "cuadrados" baratos. En su lugar, utiliza un **Acumulador de Fase** de 32 bits y una **Lookup Table (LUT)** de 32 muestras para calcular una onda sinusoidal matemáticamente perfecta en tiempo real.

### El Corazón del Sistema (Timer 1)
El `TIM1` se configura para contar a 96 MHz y reiniciarse a los 6000 ciclos (`PWM_PERIOD`).
`96,000,000 Hz / 6000 = 16,000 Hz (16 kHz)`
Esto genera una interrupción exacta a 16 kHz. En cada interrupción, el Acumulador de Fase avanza un "paso" (`phaseStep`) proporcional a la frecuencia deseada.

```c
// Fórmula del Phase Step:
uint64_t step = ((uint64_t)freq << 32) / 16000;
s_phaseStep = (uint32_t)step;
```

### Bug Documentado: Limpieza Prematura del Status Register (SR)
Durante el desarrollo se detectó que el PWM no generaba onda (quedando en un Duty Cycle estático del 50%). El error era sutil pero crítico:

```diff
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {
-   TIM1->SR = 0; // ERROR: Limpiar antes de leer el estado 
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) {
        // Cálculo DDS...
    }
+   TIM1->SR = 0; // CORRECTO: Limpiar al salir
}
```
Si se borra el registro de estado (`SR`) *antes* de evaluarlo con `TIM_GetITStatus()`, la función asume que no hubo interrupción y se salta el cálculo de la onda. 

## Gestor de Secuencias Asíncrono
La orquestación de melodías se hace en la función `AudioEngine_Task()`, la cual es llamada por el `TimeManager` cada 1 milisegundo. Esta tarea lee estructuras `ToneSequence_t` que contienen frecuencias y duraciones:

```c
static const ToneSequence_t SEQ_NEXTEL[] = {
    {1800, 20}, // Tono 1800Hz por 20ms
    {0, 20},    // Silencio
    {1800, 20}, // Tono 1800Hz por 20ms
    {0, 20},    // Silencio
    {1800, 60}, // Tono final 1800Hz por 60ms
    {0, 0}      // Fin
};
```
Cuando la secuencia termina, el sistema automáticamente:
1. Pone el `phaseStep` a 0 (silencio).
2. Apaga las salidas PWM (`TIM_CtrlPWMOutputs`).
3. Detiene el Timer para ahorrar ciclos de CPU (`TIM_Cmd`).
4. Apaga el amplificador de la bocina (`GPIOB->BRR = GPIO_Pin_2;`).
