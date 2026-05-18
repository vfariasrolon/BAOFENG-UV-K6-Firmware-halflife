import dnfile
import sys
from dncil.cil.body.reader import CilMethodBodyReader
from dncil.cil.error import CilReaderError

try:
    pe = dnfile.dnPE('BFK6_Bootloader.exe')
    
    # We want to iterate over all methods in MethodDef table
    # The MethodDef table has row_count rows
    print(f"Number of methods: {pe.net.mdtables.MethodDef.rows_count}")
    
    for row in pe.net.mdtables.MethodDef:
        if not row.Rva:
            continue
            
        try:
            # Read method body
            body = CilMethodBodyReader.read_method_body(pe, row)
            
            # Let's filter methods related to communication or booting
            name = row.Name
            # We are interested in methods inside FormMain, BootHelper, etc.
            if any(kw in name.lower() for kw in ['boot', 'handshake', 'comm', 'write', 'send', 'read', 'update', 'down', 'flash']):
                print(f"\n==========================================")
                print(f"Method: {name} (RVA: {hex(row.Rva)})")
                print(f"==========================================")
                
                # Iterate instructions
                instructions = list(body.instructions)
                for ins in instructions:
                    # Format operand nicely
                    operand_str = ""
                    if ins.operand is not None:
                        if hasattr(ins.operand, 'Name'):
                            operand_str = f"{ins.operand.Name}"
                        else:
                            operand_str = str(ins.operand)
                    
                    print(f"  {hex(ins.offset)}: {ins.opcode.name} {operand_str}")
        except Exception as e:
            print(f"Error reading method {row.Name}: {e}")
            
except Exception as e:
    print("Error:", e)
    import traceback
    traceback.print_exc()
