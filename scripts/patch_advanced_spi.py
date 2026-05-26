import re

filepath = "src/Driver/DevFD6818.c"
with open(filepath, 'r', encoding='latin-1') as f:
    content = f.read()

# Patch Rfic_WriteWord
content = re.sub(
    r'(void Rfic_WriteWord\(U8  devAddr,U16 devData\)\s*\{\s*)',
    r'\1    uartSendChar(0xFE);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n',
    content
)

# Patch Rfic_ReadWord
content = re.sub(
    r'(U16 Rfic_ReadWord\(U8  devAddr\)\s*\{\s*U16 devData;\s*)',
    r'\1',
    content
)

content = re.sub(
    r'(devData \= \(Spi_Read\(\)<<8\);.*?\s*devData \|\= Spi_Read\(\);\s*.*?Spi_CS_H\(\);\s*)',
    r'\1    uartSendChar(0xFD);\n    uartSendChar(devAddr);\n    uartSendChar(devData >> 8);\n    uartSendChar(devData & 0xFF);\n',
    content
)

with open(filepath, 'w', encoding='latin-1') as f:
    f.write(content)
print("DevFD6818 in advanced patched!")
