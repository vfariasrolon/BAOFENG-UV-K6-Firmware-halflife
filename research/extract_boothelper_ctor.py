with open('cil_disassembly.txt', 'r') as f:
    lines = f.readlines()

start_idx = -1
for i, line in enumerate(lines):
    # The first method .ctor starts around line 1
    if "Method: .ctor (RVA: 0x2050" in line or "Method: .ctor (RVA: 0x2" in line:
        start_idx = i
        break

if start_idx == -1:
    # Let's search for the first Method: .ctor in the file
    for i, line in enumerate(lines):
        if "Method: .ctor" in line:
            start_idx = i
            break

if start_idx != -1:
    print(f"Found BootHelper .ctor at line {start_idx}")
    for line in lines[start_idx:start_idx+150]:
        print(line, end='')
else:
    print("Method BootHelper .ctor not found")
