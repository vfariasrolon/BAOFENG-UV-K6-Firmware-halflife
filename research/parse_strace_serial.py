import re
import os
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
    print("🔍 ESCANEANDO TODAS LAS ESCRITURAS DE FDS NO ESTÁNDAR...")
    log_files = glob.glob("log_strace.*")
    
    for filename in log_files:
        fd_counts = {}
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                m = re.match(r'^(?:[0-9]+\s+)?write\(([0-9]+),', line)
                if m:
                    fd = int(m.group(1))
                    if fd > 2:
                        fd_counts[fd] = fd_counts.get(fd, 0) + 1
        
        if fd_counts:
            print(f"\n📄 {filename} tiene escrituras en FDs no estándar: {fd_counts}")
            
            # Let's print the first 40 write/read lines for each of these FDs
            with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
                printed = {}
                for line_num, line in enumerate(f, 1):
                    # We want to catch split writes, unfinished writes, etc.
                    m = re.match(r'^(?:[0-9]+\s+)?(write|read)\(([0-9]+),\s*"([^"]*)",\s*([0-9]+)\)', line)
                    if m:
                        op = m.group(1)
                        fd = int(m.group(2))
                        content = m.group(3)
                        length = int(m.group(4))
                        
                        if fd > 2:
                            if fd not in printed:
                                printed[fd] = 0
                            if printed[fd] < 40:
                                printed[fd] += 1
                                raw = parse_octal_escapes(content)
                                print(f"  [{op.upper()} fd={fd} len={length}] Text: {raw[:200]}... | Hex: {raw.hex()[:200]}...")
                    else:
                        m_split = re.search(r'write\(([0-9]+),\s*"([^"]*)"', line)
                        if m_split:
                            fd = int(m_split.group(1))
                            if fd > 2:
                                if fd not in printed:
                                    printed[fd] = 0
                                if printed[fd] < 40:
                                    printed[fd] += 1
                                    raw = parse_octal_escapes(m_split.group(2))
                                    print(f"  [PARTIAL WRITE fd={fd}] Text: {raw[:200]}... | Hex: {raw.hex()[:200]}... | Line: {line.strip()[:200]}...")

if __name__ == "__main__":
    main()
