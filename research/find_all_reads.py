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
    print("🔍 BUSCANDO OPERACIONES DE LECTURA EN TODOS LOS ARCHIVOS DE TRAZA...")
    log_files = glob.glob("log_strace.*")
    
    # We want to search for read(FD, "...", len) in all files.
    # We want to print lines where FD matches 22 (or whatever FD was serial).
    # Since serial FD could be different in different runs/threads, we will look for:
    # read(FD, ... that returns a string starting with \252 (0xaa).
    # Let's scan all log files!
    
    for filename in log_files:
        found_count = 0
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                # We want to match read(FD, "...", len)
                # where the content starts with \252 (which is 0xaa)
                if 'read(' in line and ('\\252' in line or '\\xaa' in line or 'read(22,' in line or 'read(9,' in line):
                    found_count += 1
                    if found_count <= 20:
                        print(f"\n✨ Lectura en {filename} L{line_num}:")
                        print(f"  Raw: {line.strip()[:300]}")
                        
                        m = re.search(r'read\(([0-9]+),\s*"([^"]*)",\s*([0-9]+)\)', line)
                        if m:
                            fd = m.group(1)
                            content = m.group(2)
                            raw = parse_octal_escapes(content)
                            print(f"  Decodificado FD {fd}: {raw.hex()}")
                            ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
                            print(f"  ASCII: {ascii_repr}")
            if found_count > 20:
                print(f"  ... ({found_count - 20} lecturas más en {filename})")

if __name__ == "__main__":
    main()
