import re

exe_path = "BFK6_Bootloader.exe"

print("🔍 Escaneando todas las cadenas en mayúsculas de BFK6_Bootloader.exe...")

with open(exe_path, "rb") as f:
    data = f.read()

# Find sequences of uppercase letters, numbers, hyphens, and underscores of length 3 to 15
matches = re.findall(b'[A-Z0-9_\-]{3,15}', data)

unique_matches = sorted(list(set(matches)))

print(f"📦 Encontradas {len(unique_matches)} cadenas únicas en mayúsculas:")
for m in unique_matches:
    try:
        s = m.decode('ascii')
        # Skip strings that are purely numeric or too common
        if not s.isdigit() and len(s) >= 4:
            print(f"👉 {s}")
    except:
        pass
