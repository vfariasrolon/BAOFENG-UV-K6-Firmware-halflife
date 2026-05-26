import re
import os

# 1. Patch BK4829_Minimal.c
filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, 'r') as f:
    content = f.read()

usart_code = """
// ==========================================
// USART SNOOPER
// ==========================================
#include "KD32f328_usart.h"
#include "KD32f328_rcc.h"

void Usart_Init(void) {
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
    
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void uartSendChar(unsigned char ch) {
    USART1->TDR = (ch & (uint16_t)0x01FF);
    uint32_t timeout = 100000;
    while (((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET) && --timeout); 
}
"""

if "Usart_Init" not in content:
    content += usart_code

content = re.sub(
    r'(void BK4829_WriteReg\(uint8_t reg, uint16_t val\)\s*\{\s*)',
    r'\1    uartSendChar(0xFE);\n    uartSendChar(reg);\n    uartSendChar(val >> 8);\n    uartSendChar(val & 0xFF);\n',
    content
)

content = re.sub(
    r'(uint16_t BK4829_ReadReg\(uint8_t reg\)\s*\{\s*uint16_t val = 0;\s*)',
    r'\1',
    content
)

content = re.sub(
    r'(return val;\s*\}\s*//)',
    r'    uartSendChar(0xFD);\n    uartSendChar(reg);\n    uartSendChar(val >> 8);\n    uartSendChar(val & 0xFF);\n    \1',
    content
)

with open(filepath, 'w') as f:
    f.write(content)

# 2. Patch main.c
filepath = "src/App/main.c"
with open(filepath, 'r') as f:
    content = f.read()

if "Usart_Init();" not in content:
    content = content.replace('WDT_Init(); // Arrancar WDT temprano\n', 'WDT_Init(); // Arrancar WDT temprano\n\n    extern void Usart_Init(void);\n    Usart_Init();\n')

with open(filepath, 'w') as f:
    f.write(content)

# 3. Patch platformio.ini
filepath = "platformio.ini"
with open(filepath, 'r') as f:
    content = f.read()

if "KD32f328_usart.c" not in content:
    content = content.replace('+<Driver/BK4829_Minimal.c>', '+<Driver/KD32f328_usart.c>\n\t+<Driver/BK4829_Minimal.c>')

with open(filepath, 'w') as f:
    f.write(content)

print("Done patching minifirm.")
