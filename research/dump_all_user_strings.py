import dnfile

pe = dnfile.dnPE('BFK6_Bootloader.exe')

print("--- USER STRINGS ---")
# User strings can be read by iterating over UserString heap bytes or using dnfile API
# In dnfile, pe.net.user_strings is a heap object
# We can print all strings from the stream
heap = pe.net.user_strings
offset = 0
while offset < heap.sizeof():
    # Read string at offset
    try:
        s = heap.get(offset)
        if s:
            print(f"Offset {hex(offset)} | Token {hex(0x70000000 | offset)}: {s}")
            # Heap items are stored as: [length encoded] + [utf16 characters] + [suffix]
            # To advance, we can inspect size or just try next offset
    except Exception:
        pass
    offset += 1
