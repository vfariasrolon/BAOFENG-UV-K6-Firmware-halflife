import re

filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, "r") as f:
    content = f.read()

content = content.replace(
    'void uartSendChar(unsigned char ch) {\n    USART1->TDR = (ch & (uint16_t)0x01FF);\n    while ((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET); \n}',
    'void uartSendChar(unsigned char ch) {\n    USART1->TDR = (ch & (uint16_t)0x01FF);\n    uint32_t timeout = 100000;\n    while (((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET) && --timeout); \n}'
)

with open(filepath, "w") as f:
    f.write(content)
