import re

filepath = "k6_rescue.py"
with open(filepath, "r") as f:
    content = f.read()

if "bfk6-minifirm-snooper.bin" not in content:
    content = content.replace('print("  [5] Salir")', 'print("  [5] Firmware Custom Half-Life: bfk6-minifirm-snooper.bin")\n    print("  [6] Salir")')
    content = content.replace('if opcion == "5":', 'elif opcion == "5":\n        target = "out/bfk6-minifirm-snooper.bin"\n    elif opcion == "6":')
    content = content.replace('Ingresa tu opción (1-5):', 'Ingresa tu opción (1-6):')

with open(filepath, "w") as f:
    f.write(content)
