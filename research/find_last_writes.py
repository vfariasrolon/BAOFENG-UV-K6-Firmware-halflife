def main():
    filename = "log_strace.721544"
    print("🔍 BUSCANDO EL FINAL DE LAS ESCRITURAS AL FD 22...")
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    write_lines = [line.strip() for line in lines if 'write(22,' in line]
    print(f"📈 Total escrituras en el log: {len(write_lines)}")
    
    # Let's print the last 15 writes with their line numbers in the original file
    idx_writes = [(i+1, line.strip()) for i, line in enumerate(lines) if 'write(22,' in line]
    
    for lnum, line in idx_writes[-10:]:
        print(f"L{lnum}: {line[:250]}")

if __name__ == "__main__":
    main()
