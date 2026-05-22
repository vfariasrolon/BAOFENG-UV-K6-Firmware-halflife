import re

filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, 'r') as f:
    content = f.read()

content = content.replace(
    'RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);',
    'RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);\n    RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);'
)

with open(filepath, 'w') as f:
    f.write(content)
print("UART clock fixed!")
