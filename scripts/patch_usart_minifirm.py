import re

filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, "r") as f:
    content = f.read()

content = content.replace(
    'RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);',
    'RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);\n    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);'
)

with open(filepath, "w") as f:
    f.write(content)
