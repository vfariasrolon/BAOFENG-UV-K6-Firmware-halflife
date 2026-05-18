import dnfile

pe = dnfile.dnPE('BFK6_Bootloader.exe')
print("--- METHOD DEFS ---")
for idx, row in enumerate(pe.net.mdtables.MethodDef):
    token = 0x06000000 | (idx + 1)
    print(f"Index {idx} | Token {hex(token)} | Name: {row.Name}")
