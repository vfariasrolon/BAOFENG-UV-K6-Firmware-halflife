with open('cil_disassembly.txt', 'r') as f:
    lines = f.readlines()

start_idx = -1
for i, line in enumerate(lines):
    if "Method: btnDownload_Click" in line:
        start_idx = i
        break

if start_idx != -1:
    print(f"Found btnDownload_Click at line {start_idx}")
    for line in lines[start_idx:start_idx+150]:
        print(line, end='')
else:
    print("Method btnDownload_Click not found")
