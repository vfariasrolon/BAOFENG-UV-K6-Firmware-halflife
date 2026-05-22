import re

filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, 'r') as f:
    content = f.read()

content = re.sub(
    r'(void BK4829_WriteReg\(uint8_t devAddr, uint16_t devData\) \{\s*)',
    r'\1    uartSendChar(0xFE);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n',
    content
)

content = re.sub(
    r'(uint16_t BK4829_ReadReg\(uint8_t devAddr\) \{\s*uint16_t devData = 0;\s*)',
    r'\1',
    content
)

content = re.sub(
    r'(return devData;\s*\})',
    r'    uartSendChar(0xFD);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n    \1',
    content
)

with open(filepath, 'w') as f:
    f.write(content)
print("Snooper code injected!")
