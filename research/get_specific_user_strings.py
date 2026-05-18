import dnfile

pe = dnfile.dnPE('BFK6_Bootloader.exe')

for t in [0xA9, 0xB7, 0xC7, 0xD3, 0xE9, 0xF5]:
    try:
        obj = pe.net.user_strings.get(t)
        print(f"Token {hex(0x70000000 | t)} type: {type(obj)}")
        # Print all directory attributes of the object
        print("  Attributes:", dir(obj))
        print("  Value:", getattr(obj, 'value', 'No value attribute'))
        print("  Str:", str(obj))
    except Exception as e:
        print(f"Token {hex(0x70000000 | t)} failed: {e}")
