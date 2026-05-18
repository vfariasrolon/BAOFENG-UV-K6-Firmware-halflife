import dnfile

pe = dnfile.dnPE('BFK6_Bootloader.exe')

# The token is 0x700000E9.
# In dnfile, UserStrings can be accessed via pe.net.user_strings
token_idx = 0x000000E9
us = pe.net.user_strings.get_str(token_idx)
print(f"Token 0x700000E9 UserString: {us}")

# Let's also print 0x700000A9, 0x700000B7, 0x700000C7, 0x700000D3
for t in [0xA9, 0xB7, 0xC7, 0xD3, 0xE9, 0xF5]:
    try:
        val = pe.net.user_strings.get_str(t)
        print(f"Token {hex(0x70000000 | t)}: {val}")
    except Exception as e:
        print(f"Token {hex(0x70000000 | t)} failed: {e}")
