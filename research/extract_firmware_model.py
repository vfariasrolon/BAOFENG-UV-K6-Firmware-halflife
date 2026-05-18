import os
import re

bin_dir = 'out/'
bin_files = [f for f in os.listdir(bin_dir) if f.startswith('bfk6-') and f.endswith('.bin')]
if not bin_files:
    print("❌ No firmware files found in out/")
    sys.exit(1)
    
bin_files.sort(reverse=True)
bin_path = os.path.join(bin_dir, bin_files[0])

print(f"📖 Scanning firmware binary: {bin_path}")

with open(bin_path, 'rb') as f:
    data = f.read()

# Search for any ASCII string of length 4 to 20 starting with BF
strings = re.findall(b'[a-zA-Z0-9_\-]{4,20}', data)
for s in strings:
    s_str = s.decode('ascii', errors='ignore')
    if s_str.startswith('BF') or 'K6' in s_str or 'NORMAL' in s_str:
        print(f"👉 Found string: {s_str}")
