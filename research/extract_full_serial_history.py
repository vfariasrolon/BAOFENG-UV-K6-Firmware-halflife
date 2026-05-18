import re

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
    filename = "log_strace.721544"
    print(f"📡 EXTRACCIÓN TOTAL DE LA COMUNICACIÓN SERIAL EN {filename}...")
    
    events = []
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            # Check for any write or read on FD 22
            m = re.match(r'^(?:[0-9]+\s+)?(write|read)\(22,\s*"([^"]*)"', line)
            if m:
                op = m.group(1).upper()
                content = m.group(2)
                raw = parse_octal_escapes(content)
                events.append((line_num, op, raw))
                
    print(f"📊 Se encontraron {len(events)} transacciones seriales en FD 22.")
    
    # We want to print the handshake (the first 40 packets)
    print("\n🤝 --- INICIO DE LA COMUNICACIÓN (HANDSHAKE) ---")
    for idx, (line_num, op, raw) in enumerate(events[:40]):
        direction = "➡️ ENVIADO" if op == "WRITE" else "⬅️ RECIBIDO"
        print(f"[{idx+1}] L{line_num} {direction} (len={len(raw)}):")
        print(f"    Hex : {raw.hex()}")
        ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
        print(f"    Text: {ascii_repr}")
        
    # Let's also print the last 10 packets to see how it ends
    if len(events) > 40:
        print("\n🔚 --- FIN DE LA COMUNICACIÓN ---")
        for idx, (line_num, op, raw) in enumerate(events[-10:]):
            real_idx = len(events) - 10 + idx
            direction = "➡️ ENVIADO" if op == "WRITE" else "⬅️ RECIBIDO"
            print(f"[{real_idx+1}] L{line_num} {direction} (len={len(raw)}):")
            print(f"    Hex : {raw.hex()}")
            ascii_repr = "".join([chr(b) if 32 <= b <= 126 else "." for b in raw])
            print(f"    Text: {ascii_repr}")

if __name__ == "__main__":
    main()
