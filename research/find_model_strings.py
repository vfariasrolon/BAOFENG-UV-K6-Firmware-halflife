import re

with open('BFK6_Bootloader.exe', 'rb') as f:
    data = f.read()

# Find strings starting with "BF-" or "PROGRAM"
matches = re.findall(b'BF-[A-Za-z0-9_-]+', data)
for m in set(matches):
    print("Found Model:", m.decode('ascii', errors='ignore'))

# Let's also scan all ASCII strings of length 4 to 20
ascii_strings = re.findall(b'[a-zA-Z0-9_-]{4,20}', data)
print("\nSome other strings:")
for s in set(ascii_strings):
    s_dec = s.decode('ascii', errors='ignore')
    if 'K6' in s_dec or 'BF' in s_dec or 'PROGRAM' in s_dec or 'UPDATE' in s_dec:
        print("  ", s_dec)
