import re

filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, "r") as f:
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
    while ((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET); 
}

"""

if "Usart_Init" not in content:
    content = content.replace('// ==========================================\n// UTILIDADES SPI', usart_code + '// ==========================================\n// UTILIDADES SPI')

content = re.sub(
    r'(void BK4829_WriteReg\(uint8_t devAddr, uint16_t devData\) \{\s*)',
    r'\1    uartSendChar(0xFE);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n',
    content
)

content = re.sub(
    r'(if \(devData == 0x0000 \|\| devData == 0xFFFF\) \{\s*retries\+\+;\s*if \(retries <= 3\) \{\s*BK4829_ResetBus\(\);\s*goto retry;\s*\}\s*\}\s*)',
    r'\1    uartSendChar(0xFD);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n',
    content
)

with open(filepath, "w") as f:
    f.write(content)
