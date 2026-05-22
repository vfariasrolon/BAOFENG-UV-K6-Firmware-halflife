#include "../Common/includes.h"
#include "AudioEngine.h"
#include "TimeManager.h"
#include "../Driver/inc/KD32f328_tim.h"
#include "../Driver/inc/KD32f328_gpio.h"
#include "../Driver/inc/KD32f328_rcc.h"
#include "../Driver/inc/KD32f328_misc.h"

#define PWM_PERIOD 6000

// Lookup Table de Seno (32 muestras), escalado para un PWM de 0 a 6000
static const uint16_t s_sineLUT[32] = {
    3000, 3585, 4140, 4635, 5046, 5354, 5543, 5600,
    5543, 5354, 5046, 4635, 4140, 3585, 3000, 2415,
    1860, 1365,  954,  646,  457,  400,  457,  646,
     954, 1365, 1860, 2415, 3000, 3585, 4140, 4635 // Simétrico y suavizado
};

// --- Variables del Acumulador de Fase ---
static volatile uint32_t s_phaseAcc = 0;
static volatile uint32_t s_phaseStep = 0;
static volatile uint32_t s_samplesRemaining = 0;

// Estructura de secuencia
typedef struct {
    uint16_t freq;
    uint16_t duration_ms;
} ToneSequence_t;

static const ToneSequence_t* s_currentSequence = NULL;
static volatile uint8_t s_sequenceIndex = 0;
static volatile uint32_t s_sequenceTimer = 0;

// --- Definición de Secuencias ---
// Secuencia Nextel Chirp
static const ToneSequence_t SEQ_NEXTEL[] = {
    {1800, 20}, // Tono
    {0, 20},    // Silencio
    {1800, 20}, // Tono
    {0, 20},    // Silencio
    {1800, 60}, // Tono final largo
    {0, 0}      // Fin de secuencia
};

// Secuencia Beep Éxito
static const ToneSequence_t SEQ_AUTH_SUCCESS[] = {
    {1500, 40},
    {2000, 40},
    {2500, 80},
    {0, 0}
};

static const ToneSequence_t SEQ_ACK[] = {
    {2500, 100}, // Beep corto 100ms
    {0, 0}
};

void AudioEngine_Init(void) {
    // 1. Relojes
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 2. GPIO PA11 en modo Alternate Function 2 (TIM1_CH4) para Audio PWM
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_2);
    
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Inicializar PB2 como Salida Push-Pull (Speaker Switch / Amplifier Enable)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // Apagar altavoz por defecto
    GPIOB->BRR = GPIO_Pin_2;

    // 3. Configuración de TIM1 Base (16 kHz con 96 MHz)
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_TimeBaseStructure.TIM_Prescaler = 0;   
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;   
    TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD - 1;          
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;    
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // 4. Configuración del PWM en CH4
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;    
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_Pulse = 3000; // 50% duty inicial
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);  
    
    // 5. Habilitar Interrupción de Update (TIM1_UP)
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_UP_TRG_COM_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 1; // Alta prioridad
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Apagar Timer inicialmente
    TIM_Cmd(TIM1, DISABLE);
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
}

// Configura la frecuencia del generador
static void SetTone(uint16_t freq) {
    if (freq == 0) {
        s_phaseStep = 0;
        TIM_SetCompare4(TIM1, 0); // Silencio
        return;
    }
    // phaseStep = (freq * 2^32) / 16000
    // Equivalente seguro en enteros:
    uint64_t step = ((uint64_t)freq << 32) / 16000;
    s_phaseStep = (uint32_t)step;
}

// Inicia una secuencia
static void PlaySequence(const ToneSequence_t* seq) {
    s_currentSequence = seq;
    s_sequenceIndex = 0;
    s_sequenceTimer = g_SystemTick;
    
    // Iniciar primer tono
    SetTone(s_currentSequence[0].freq);
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    // Habilitar bocina
    GPIOB->BSRR = GPIO_Pin_2; // Speaker ON
}

void AudioEngine_PlayNextelChirp(void) {
    PlaySequence(SEQ_NEXTEL);
}

void AudioEngine_PlayAuthBeep(void) {
    PlaySequence(SEQ_AUTH_SUCCESS);
}

void AudioEngine_PlayAck(void) {
    PlaySequence(SEQ_ACK);
}

// Interrupción que corre a 16 kHz exactos
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

        // --- Generador de Audio ---
        if (s_currentSequence != NULL) {
            // Acumulador de Fase
            s_phaseAcc += s_phaseStep;
            
            // Si hay un tono activo (step > 0), generar PWM
            if (s_phaseStep > 0) {
                // Obtener los 5 bits más significativos para indexar la LUT de 32 elementos
                uint32_t index = s_phaseAcc >> 27;
                TIM1->CCR4 = s_sineLUT[index];
            }
        }
    }
    TIM1->SR = 0; // Limpiar al final
}

// Tarea en el main loop que actualiza la secuencia
void AudioEngine_Task(void) {
    if (s_currentSequence != NULL) {
        uint32_t currentDuration = s_currentSequence[s_sequenceIndex].duration_ms;
        
        if ((g_SystemTick - s_sequenceTimer) >= currentDuration) {
            // Siguiente paso
            s_sequenceIndex++;
            s_sequenceTimer = g_SystemTick;
            
            if (s_currentSequence[s_sequenceIndex].duration_ms == 0) {
                // Fin de secuencia
                s_currentSequence = NULL;
                SetTone(0);
                TIM_CtrlPWMOutputs(TIM1, DISABLE);
                TIM_Cmd(TIM1, DISABLE); // Apagar timer para ahorrar CPU
                
                // No apagamos el amplificador nunca por ahora para asegurar 
                // que el Squelch y los tonos DTMF puedan ser escuchados.
                // GPIOB->BRR = GPIO_Pin_2; // Speaker OFF
            } else {
                // Nuevo tono
                SetTone(s_currentSequence[s_sequenceIndex].freq);
            }
        }
    }
}
