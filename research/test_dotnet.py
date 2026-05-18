import dotnetfile
import sys

try:
    pe = dotnetfile.DotNetPE('BFK6_Bootloader.exe')
    print("PE attributes:")
    for attr in dir(pe):
        if not attr.startswith('__'):
            print(f" - {attr}")
            
except Exception as e:
    print("Error:", e)
