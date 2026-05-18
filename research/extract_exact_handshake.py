import re
import os

def extract_exact_handshake():
    exe_path = "BFK6_Bootloader.exe"
    if not os.path.exists(exe_path):
        print(f"❌ No se encuentra el archivo {exe_path} en el directorio actual.")
        return
        
    print("🧠 Escaneando estructura binaria profunda de BFK6_Bootloader.exe...")
    
    with open(exe_path, "rb") as f:
        data = f.read()
        
    # Buscamos la palabra 'PROGRAM' en el binario completo
    matches = [m.start() for m in re.finditer(b"PROGRAM", data)]
    
    if not matches:
        print("❌ No se encontró la secuencia base 'PROGRAM'.")
        return
        
    print(f"🔍 Encontradas {len(matches)} ocurrencias de 'PROGRAM'. Analizando vecindad de bytes...")
    
    for idx, offset in enumerate(matches):
        # Extraemos una ventana de 64 bytes alrededor de donde empieza 'PROGRAM'
        window = data[offset : offset + 64]
        print(f"\n--- Ocurrencia {idx+1} (Offset: {hex(offset)}) ---")
        print(f"Crudo (Hex): {window.hex()}")
        
        # Intentamos decodificar caracteres legibles y de control
        readable = []
        for b in window:
            if 32 <= b <= 126:
                readable.append(chr(b))
            else:
                readable.append(f"\\x{b:02x}")
        print(f"Legible: {''.join(readable)}")

if __name__ == "__main__":
    extract_exact_handshake()
