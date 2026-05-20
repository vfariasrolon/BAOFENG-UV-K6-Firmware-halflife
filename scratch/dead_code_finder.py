import os
import re

src_dir = "/home/maxheadroom/Proyectos/BAOFENG-UV-K6-Firmware-halflife/src"

# Regex to find function definitions in C across multiple lines
func_pattern = re.compile(
    r'(?:extern\s+|static\s+)?(?:void|U8|U16|U32|int|char|Boolean|float|double|S16|S32)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:\{|\n\s*\{)',
    re.MULTILINE
)

# Store all defined functions: function_name -> (file_path, line_number)
defined_functions = {}
file_contents = {}

# 1. Read all files and find all function definitions
for root, dirs, files in os.walk(src_dir):
    for file in files:
        if file.endswith(('.c', '.h')):
            path = os.path.join(root, file)
            try:
                with open(path, 'r', encoding='gbk', errors='ignore') as f:
                    content = f.read()
                file_contents[path] = content
                
                # If it's a .c file, extract definitions
                if file.endswith('.c'):
                    # Use finditer to search the whole file content
                    for match in func_pattern.finditer(content):
                        func_name = match.group(1)
                        if func_name != 'main':
                            # Find the line number of this match
                            start_idx = match.start()
                            line_number = content[:start_idx].count('\n') + 1
                            defined_functions[func_name] = (path, line_number)
            except Exception as e:
                pass

print(f"🔍 Se encontraron {len(defined_functions)} funciones definidas en total.")
print("🕵️ Analizando referencias de cada función...")

orphan_functions = []

# 2. Check references for each defined function
for func_name, (def_path, def_line) in defined_functions.items():
    ref_count = 0
    ref_details = []
    
    # We search for the function name as a full word: \bfunc_name\b
    word_pattern = re.compile(rf'\b{re.escape(func_name)}\b')
    
    for path, content in file_contents.items():
        # Count references in this file
        lines = content.splitlines()
        for idx, line in enumerate(lines):
            # Skip the definition line itself
            if path == def_path and (idx + 1) == def_line:
                continue
                
            # Skip lines that look like comments or empty space
            trimmed = line.strip()
            if not trimmed or trimmed.startswith('//') or trimmed.startswith('*') or trimmed.startswith('/*'):
                continue
                
            if word_pattern.search(line):
                # Check if it's just a header declaration or extern
                is_decl = False
                if path.endswith('.h') or 'extern' in line:
                    is_decl = True
                
                ref_count += 1
                ref_details.append((path, idx + 1, line.strip(), is_decl))
                
    # Filter out declarations
    non_decl_refs = [ref for ref in ref_details if not ref[3]]
    
    if len(non_decl_refs) == 0:
        orphan_functions.append((func_name, def_path, def_line, ref_details))

# 3. Print report
print("\n" + "="*80)
print(f"🛑 REPORTE DE FUNCIONES HUÉRFANAS (SIN USO) DETECTADAS: {len(orphan_functions)}")
print("="*80)

for idx, (func_name, path, line, refs) in enumerate(sorted(orphan_functions, key=lambda x: x[1])):
    rel_path = os.path.relpath(path, src_dir)
    print(f"{idx+1}. 🔴 {func_name}()")
    print(f"   📍 Definida en: {rel_path}:{line}")
    if refs:
        print("   📎 Declaraciones encontradas:")
        for ref_path, ref_line, ref_content, _ in refs:
            ref_rel = os.path.relpath(ref_path, src_dir)
            print(f"      - {ref_rel}:{ref_line} -> {ref_content}")
    else:
        print("   📎 Sin ninguna otra referencia o declaración.")
    print("-" * 50)
