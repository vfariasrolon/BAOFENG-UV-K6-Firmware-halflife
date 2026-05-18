import dnfile
import dncil.cil.body.reader

pe = dnfile.dnPE('BFK6_Bootloader.exe')

def disassemble_method(idx, name):
    row = pe.net.mdtables.MethodDef[idx]
    print(f"\nMethod {name} (Token {hex(0x06000000 | (idx + 1))}):")
    offset = pe.get_offset_from_rva(row.Rva)
    with open('BFK6_Bootloader.exe', 'rb') as f:
        f.seek(offset)
        raw_bytes = f.read()
    body = dncil.cil.body.reader.read_method_body_from_bytes(raw_bytes)
    for ins in body.instructions:
        print(f"  {hex(ins.offset)}: {ins.opcode.name} {ins.operand}")

disassemble_method(17, "get_Command")
disassemble_method(19, "get_CommandArgs")
