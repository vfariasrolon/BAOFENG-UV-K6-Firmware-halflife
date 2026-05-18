import re
import glob

def parse_octal_escapes(s):
    res = bytearray()
    i = 0
    n = len(s)
    while i < n:
        if s[i] == '\\':
            if i + 1 >= n:
                res.append(ord('\\'))
                break
            next_char = s[i+1]
            if next_char in '01234567':
                oct_str = ""
                j = 0
                while i + 1 + j < n and j < 3 and s[i+1+j] in '01234567':
                    oct_str += s[i+1+j]
                    j += 1
                val = int(oct_str, 8)
                res.append(val)
                i += 1 + j
            else:
                escapes = {
                    'n': 10, 'r': 13, 't': 9, 'b': 8, 'a': 7, 'f': 12, 'v': 11,
                    '"': 34, '\\': 92, '\'': 39
                }
                if next_char in escapes:
                    res.append(escapes[next_char])
                else:
                    res.append(ord(next_char))
                i += 2
        else:
            res.append(ord(s[i]))
            i += 1
    return bytes(res)

def main():
    print("🔍 BUSCANDO PAQUETES BAOFENG (OCTAL \\252) EN TODOS LOS LOGS...")
    log_files = glob.glob("log_strace.*")
    
    found_any = False
    for filename in log_files:
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                # Search for write(FD, "\252... or write(FD, "\xaa...
                # Baofeng packets start with 0xaa (octal \252)
                if '\\252' in line or '\\xaa' in line:
                    found_any = True
                    print(f"\n✨ ¡ENCONTRADO EN {filename} LÍNEA {line_num}! ✨")
                    print(f"Línea cruda: {line.strip()[:300]}...")
                    
                    # Try to extract the fd and the string
                    m = re.search(r'write\(([0-9]+),\s*"([^"]*)"', line)
                    if m:
                        fd = int(m.group(1))
                        content = m.group(2)
                        raw = parse_octal_escapes(content)
                        print(f"  -> FD: {fd}")
                        print(f"  -> Largo decodificado: {len(raw)} bytes")
                        print(f"  -> Bytes Hex: {raw.hex()}")
                        ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
                        print(f"  -> ASCII: {ascii_repr}")
                        
                        # Let's search if there are other reads/writes on this same FD in this file!
                        print(f"  -> Buscando otros eventos en {filename} para FD {fd}:")
                        search_fd(filename, fd)

def search_fd(filename, fd):
    pattern = f"({fd},"
    count = 0
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            if pattern in line and ('write' in line or 'read' in line):
                count += 1
                if count <= 15:
                    op = "WRITE" if "write" in line else "READ"
                    m = re.search(r'(?:write|read)\([0-9]+,\s*"([^"]*)"', line)
                    if m:
                        raw = parse_octal_escapes(m.group(1))
                        print(f"    [{op} L{line_num}] Hex: {raw.hex()[:100]} | ASCII: {''.join([chr(b) if 32 <= b <= 126 else '.' for b in raw])[:100]}")
                    else:
                        print(f"    [{op} L{line_num}] Línea cruda: {line.strip()[:100]}")
                elif count == 16:
                    print("    ... (más eventos omitidos)")

if __name__ == "__main__":
    main()
