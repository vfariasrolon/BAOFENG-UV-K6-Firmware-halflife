import re

filepath = "src/Core/main.c"
with open(filepath, "r") as f:
    content = f.read()

if "Usart_Init" not in content:
    content = content.replace('void main(void)\n{', 'extern void Usart_Init(void);\n\nvoid main(void)\n{\n    Usart_Init();')

with open(filepath, "w") as f:
    f.write(content)
