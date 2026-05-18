import dotnetfile

pe = dotnetfile.DotNetPE('BFK6_Bootloader.exe')
pe.parse_all()

for i, table in enumerate(pe.metadata_tables):
    if table is not None and len(table.table_rows) > 0:
        print(f"\n=== Table index {i} ({len(table.table_rows)} rows) ===")
        # Print string representation of the first few rows
        for r_idx, row in enumerate(table.table_rows[:10]):
            print(f"Row {r_idx}: {row}")
            # If the row has attributes, print them
            attrs = [attr for attr in dir(row) if not attr.startswith('_')]
            print(f"  Attributes: {attrs}")
            for attr in attrs:
                try:
                    val = getattr(row, attr)
                    if attr == 'RVA':
                        print(f"    {attr}: {hex(val)}")
                    else:
                        print(f"    {attr}: {val}")
                except Exception:
                    pass
        if len(table.table_rows) > 10:
            print("... (truncated)")
