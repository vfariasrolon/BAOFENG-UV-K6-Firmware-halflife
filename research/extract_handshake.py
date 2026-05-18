import re
import os

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
                # Read up to 3 octal digits
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
    filename = "log_strace.721343"
    print(f"📊 EXTRAYENDO HISTORIAL DE COMUNICACIÓN DE {filename}...")
    
    # We want to trace all writes and reads on FD 22 and FD 9.
    # We will print them in order.
    
    events = []
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            # Check for write/read
            m_full = re.match(r'^(?:[0-9]+\s+)?(write|read)\(([0-9]+),\s*"([^"]*)",\s*([0-9]+)\)', line)
            if m_full:
                op = m_full.group(1).upper()
                fd = int(m_full.group(2))
                content = m_full.group(3)
                length = int(m_full.group(4))
                if fd in [9, 22]:
                    raw = parse_octal_escapes(content)
                    events.append((line_num, op, fd, raw, length))
            else:
                m_split = re.search(r'^(?:[0-9]+\s+)?(write|read)\(([0-9]+),\s*"([^"]*)"', line)
                if m_split:
                    op = m_split.group(1).upper()
                    fd = int(m_split.group(2))
                    content = m_split.group(3)
                    if fd in [9, 22]:
                        raw = parse_octal_escapes(content)
                        events.append((line_num, op + "_PARTIAL", fd, raw, len(raw)))

    print(f"📈 Total de eventos encontrados en puertos seriales: {len(events)}")
    
    # Let's print the first 50 events in detail
    print("\n📋 DETALLE DE LOS PRIMEROS 50 EVENTOS:")
    for idx, (line_num, op, fd, raw, length) in enumerate(events[:50]):
        print(f"Event #{idx+1} [Línea {line_num}] | {op} fd={fd} (anunciado len={length}, decodificado len={len(raw)}):")
        print(f"  Hex : {raw.hex()}")
        # Check if it has ASCII printable chars
        ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
        print(f"  Text: {ascii_repr}")

if __name__ == "__main__":
    main()
