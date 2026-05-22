filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, 'r') as f:
    content = f.read()

content = content.replace("BK4829_WriteReg(0x38, (uint16_t)g_test_freq);", "BK4829_WriteReg(0x38, 0xC83D);")
content = content.replace("BK4829_WriteReg(0x39, (uint16_t)(g_test_freq >> 16));", "BK4829_WriteReg(0x39, 0x0294);")

with open(filepath, 'w') as f:
    f.write(content)
print("Freq fixed!")
