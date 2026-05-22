import re

with open("src/Driver/BK4829_Minimal.c", "r") as f:
    content = f.read()

# 1. Fix BK4829_TxEnable (remove duplicate 0x30)
content = content.replace("BK4829_WriteReg(0x30, 0x0003);\n        BK4829_WriteReg(0x30, 0xC1FE); // MODO TX ACTIVO NATIVO (0xC1FE)", "BK4829_WriteReg(0x30, 0xC1FE); // MODO TX ACTIVO NATIVO (0xC1FE)")

# 2. Fix the mess in BK4829_Init
# First, restore BK4829_Init
mess = """    // Reloj de Referencia    uint16_t temp = BK4829_ReadReg(0x70) & 0x70FF;
    temp = temp | 0x8000 | ((60 % 128) << 8); // Gain
    BK4829_WriteReg(0x70, temp);
    
    BK4829_WriteReg(0x30, 0x0302);
    
    // AFOUT Beep Out for TX/Localestro del Squelch por defecto"""
correct_init = """    // Reloj de Referencia / Cristal
    BK4829_WriteReg(0x01, 0x3FF0);
    
    // Micrófono y ganancia
    BK4829_WriteReg(0x19, 0x1041);
    BK4829_WriteReg(0x7D, 0xE952);
    
    // Registro Maestro del Squelch por defecto"""
content = content.replace(mess, correct_init)

# 3. Fix BK4829_SendDTMF 0x0002 to 0x0302
send_dtmf = """    uint16_t temp = BK4829_ReadReg(0x70) & 0x70FF;
    temp = temp | 0x8000 | ((60 % 128) << 8); // Gain
    BK4829_WriteReg(0x70, temp);
    
    BK4829_WriteReg(0x30, 0x0002);
    
    // AFOUT Beep Out for TX/Local"""
fixed_dtmf = """    uint16_t temp = BK4829_ReadReg(0x70) & 0x70FF;
    temp = temp | 0x8000 | ((60 % 128) << 8); // Gain
    BK4829_WriteReg(0x70, temp);
    
    BK4829_WriteReg(0x30, 0x0302); // TONE NATIVO
    
    // AFOUT Beep Out for TX/Local"""
content = content.replace(send_dtmf, fixed_dtmf)

# Write back
with open("src/Driver/BK4829_Minimal.c", "w") as f:
    f.write(content)
