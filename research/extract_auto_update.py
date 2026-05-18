with open('cil_disassembly.txt', 'r') as f:
    lines = f.readlines()

start_idx = -1
for i, line in enumerate(lines):
    if "Method: btn_AutoUpdate_Click" in line:
        start_idx = i
        break

if start_idx != -1:
    print(f"Found btn_AutoUpdate_Click at line {start_idx}")
    for line in lines[start_idx:start_idx+120]:
        print(line, end='')
else:
    print("Method btn_AutoUpdate_Click not found")
