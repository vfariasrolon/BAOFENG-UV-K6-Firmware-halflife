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

import re

def main():
    filename = "log_strace.721544"
    print(f"🔍 ANALIZANDO TODAS LAS ESCRITURAS AL FD 22 EN {filename}...")
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    write_count = 0
    for idx, line in enumerate(lines, 1):
        if 'write(22,' in line:
            write_count += 1
            # Parse the content
            m = re.search(r'write\(22,\s*"([^"]*)",\s*([0-9]+)\)', line)
            if m:
                content = m.group(1)
                length = m.group(2)
                raw = parse_octal_escapes(content)
                print(f"\n✍️ Escritura #{write_count} en L{idx}: (Largo {length})")
                print(f"  HEX: {raw.hex()}")
                ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
                print(f"  ASCII: {ascii_repr}")
            else:
                print(f"✍️ Escritura #{write_count} en L{idx} (No parseada): {line.strip()[:150]}")

if __name__ == "__main__":
    main()
