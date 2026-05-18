import re
import glob

def main():
    print("🔍 BUSCANDO TODAS LAS LLAMADAS A FDS DE SERIAL...")
    log_files = glob.glob("log_strace.*")
    
    # We want to find which FDs were serial.
    # In log_strace.721544, FD 22 had:
    # write(22, "\252\1...", 18)
    # ioctl(22, FIONREAD, [8])
    
    # Let's search all log files for calls on FD 22 or 9.
    # We will print the surrounding lines of FIONREAD.
    
    for filename in log_files:
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            
        for i, line in enumerate(lines):
            if 'FIONREAD' in line:
                print(f"\n✨ Encontrado FIONREAD en {filename} L{i+1}:")
                # Print 5 lines before and 5 lines after
                start = max(0, i - 3)
                end = min(len(lines), i + 4)
                for j in range(start, end):
                    prefix = "--> " if j == i else "    "
                    print(f"{prefix}L{j+1}: {lines[j].strip()}")

if __name__ == "__main__":
    main()
