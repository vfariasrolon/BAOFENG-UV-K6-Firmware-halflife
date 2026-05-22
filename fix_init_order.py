import re

filepath = "src/App/main.c"
with open(filepath, 'r') as f:
    content = f.read()

# Eliminar Usart_Init de arriba
content = content.replace("    SystemCoreClockUpdate();\n    extern void Usart_Init(void);\n    Usart_Init();\n", "")
content = content.replace("    extern void Usart_Init(void);\n    Usart_Init();\n", "")

# Insertarlo justo despues de __enable_irq()
content = content.replace("    __enable_irq();\n", "    __enable_irq();\n\n    SystemCoreClockUpdate();\n    extern void Usart_Init(void);\n    Usart_Init();\n")

with open(filepath, 'w') as f:
    f.write(content)
print("Init order fixed!")
