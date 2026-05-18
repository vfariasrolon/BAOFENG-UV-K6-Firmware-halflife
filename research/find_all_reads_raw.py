import re

def main():
    filename = "log_strace.721544"
    print(f"🔍 BUSCANDO TODAS LAS LECTURAS Y RECEPCIONES EN {filename}...")
    
    # We want to search for any read, recv, recvfrom, readv, select, poll system calls.
    # We will print the line number, raw content, and try to parse the hex.
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        for line_num, line in enumerate(f, 1):
            if any(call in line for call in ['read(', 'recv(', 'recvfrom(', 'readv(']):
                print(f"L{line_num}: {line.strip()[:200]}")

if __name__ == "__main__":
    main()
