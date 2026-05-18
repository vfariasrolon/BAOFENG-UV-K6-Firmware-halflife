import re
import os

def search_all_handshakes():
    exe_path = "BFK6_Bootloader.exe"
    if not os.path.exists(exe_path):
        print(f"❌ No se encuentra el archivo {exe_path}")
        return
        
    print("🔎 Escaneando BFK6_Bootloader.exe en busca de todas las firmas posibles...")
    with open(exe_path, "rb") as f:
        data = f.read()
        
    # Search for standard 8-bit ASCII patterns
    ascii_strings = re.findall(b'[a-zA-Z0-9_\-\x00]{4,30}', data)
    print("\n--- Cadenas ASCII/Binarias candidatas encontradas ---")
    found_any = False
    for s in ascii_strings:
        # Filter nulls for display but keep them to analyze
        clean_s = s.replace(b'\x00', b'').decode('ascii', errors='ignore')
        if clean_s.startswith('BF') or 'K6' in clean_s or 'PROGRAM' in clean_s or 'UPDATE' in clean_s:
            print(f"👉 Crudo (Hex): {s.hex().ljust(40)} | Legible: {clean_s} (Longitud cruda: {len(s)})")
            found_any = True
            
    # Also search for UTF-16 / Unicode strings (common in .NET metadata)
    unicode_strings = re.findall(b'(?:[a-zA-Z0-9_\-\x00]\x00){4,30}', data)
    print("\n--- Cadenas Unicode/UTF-16 candidatas encontradas ---")
    for u in unicode_strings:
        try:
            clean_u = u.decode('utf-16le', errors='ignore')
            if clean_u.startswith('BF') or 'K6' in clean_u or 'PROGRAM' in clean_u or 'UPDATE' in clean_u:
                print(f"👉 Crudo (Hex): {u.hex().ljust(40)} | Legible: {clean_u}")
                found_any = True
        except:
            pass

    if not found_any:
        print("❌ No se encontraron firmas coincidentes.")

if __name__ == "__main__":
    search_all_handshakes()
