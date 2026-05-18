import dotnetfile

pe = dotnetfile.DotNetPE('BFK6_Bootloader.exe')
pe.parse_all()

field_rva_table = pe.metadata_tables[15]
row = field_rva_table.table_rows[0]

print("RVA type:", type(row.RVA))
print("RVA dir:", dir(row.RVA))
try:
    print("RVA int val:", int(row.RVA))
except Exception as e:
    print("RVA int cast error:", e)

try:
    print("RVA .value:", row.RVA.value)
except Exception as e:
    print("RVA .value error:", e)
