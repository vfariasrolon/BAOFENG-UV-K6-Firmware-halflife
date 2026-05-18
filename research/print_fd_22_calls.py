def main():
    filename = "log_strace.721544"
    print(f"🔍 BUSCANDO TODAS LAS LLAMADAS AL FD 22 EN {filename}...")
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        count = 0
        for line_num, line in enumerate(f, 1):
            if '(22,' in line or ', 22,' in line or ' 22,' in line or 'ioctl(22' in line or 'write(22' in line or 'read(22' in line:
                count += 1
                if count <= 50:
                    print(f"L{line_num}: {line.strip()}")
        print(f"📈 Total de llamadas en FD 22: {count}")

if __name__ == "__main__":
    main()
