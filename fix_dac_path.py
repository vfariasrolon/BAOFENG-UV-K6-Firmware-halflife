filepath = "src/Driver/BK4829_Minimal.c"
with open(filepath, 'r') as f:
    content = f.read()

content = content.replace("BK4829_WriteReg(0x47, 0x6042);", "BK4829_WriteReg(0x47, 0x6142);")

with open(filepath, 'w') as f:
    f.write(content)
print("DAC path fixed!")
