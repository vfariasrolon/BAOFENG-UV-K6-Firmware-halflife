import re

def main():
    filename = "KDH_Bootloader.FormMain.resources"
    print(f"🔍 Analizando {filename}...")
    
    with open(filename, 'rb') as f:
        data = f.read()
        
    print(f"Tamaño total del recurso: {len(data)} bytes")
    
    # Search for common strings or names in resources
    # Resource format often stores names as UTF-8 or UTF-16 strings
    # Let's extract all printable strings of length >= 4
    strings = []
    current = bytearray()
    for b in data:
        if 32 <= b <= 126:
            current.append(b)
        else:
            if len(current) >= 4:
                strings.append(current.decode('ascii', errors='ignore'))
            current = bytearray()
            
    print("📋 Cadenas legibles encontradas (primeras 50):")
    for s in strings[:50]:
        if any(keyword in s.lower() for keyword in ["bin", "hex", "firmware", "k6", "rom", "data", "resource", "file"]):
            print(f"   ⭐ {s}")
        else:
            print(f"   {s}")

if __name__ == "__main__":
    main()
