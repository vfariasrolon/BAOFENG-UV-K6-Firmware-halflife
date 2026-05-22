#!/usr/bin/env python3
import serial
import time
import sys
import os

def crc16_ccitt(data: bytes) -> int:
    """Standard CRC-16 CCITT (XMODEM) with polynomial 0x1021, initialized to 0."""
    crc = 0
    poly = 0x1021
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

def pack_k6_packet(cmd: int, arg: int, payload: bytes = b'') -> bytes:
    """Packs a command into the Baofeng UV-K6 official bootloader frame:
    [0xAA, CMD, ARG, LEN_MSB, LEN_LSB, ...DATA..., CRC_MSB, CRC_LSB, 0xEF]
    """
    length = len(payload)
    header = bytes([cmd, arg, (length >> 8) & 0xFF, length & 0xFF]) + payload
    crc = crc16_ccitt(header)
    packet = bytes([0xAA]) + header + bytes([(crc >> 8) & 0xFF, crc & 0xFF, 0xEF])
    return packet

def read_k6_response(ser, timeout=1.5):
    """Reads a response package from the radio and verifies its structure."""
    start = time.time()
    # Read header (first 5 bytes): [0xAA, CMD, ARG, LEN_MSB, LEN_LSB]
    header = bytearray()
    while time.time() - start < timeout:
        if ser.in_waiting >= 5:
            header = ser.read(5)
            break
        time.sleep(0.005)
        
    if len(header) < 5 or header[0] != 0xAA:
        if len(header) > 0:
            print(f"   ⚠️ Encabezado inválido recibido: {header.hex()}")
        return None
        
    cmd = header[1]
    arg = header[2]
    payload_len = (header[3] << 8) | header[4]
    
    # Remaining bytes: payload_len + 2 bytes CRC + 1 byte Footer (0xEF)
    remaining_len = payload_len + 3
    body = bytearray()
    start = time.time()
    while time.time() - start < timeout and len(body) < remaining_len:
        if ser.in_waiting > 0:
            body.extend(ser.read(ser.in_waiting))
        time.sleep(0.005)
        
    if len(body) < remaining_len:
        print(f"   ⚠️ Paquete incompleto recibido. Faltan {remaining_len - len(body)} bytes.")
        return None
        
    payload = body[:payload_len]
    crc_received = (body[payload_len] << 8) | body[payload_len + 1]
    footer = body[payload_len + 2]
    
    if footer != 0xEF:
        print(f"   ⚠️ Footer inválido: {hex(footer)}")
        return None
        
    # Verify CRC
    header_for_crc = bytes(header[1:]) + bytes(payload)
    crc_calc = crc16_ccitt(header_for_crc)
    if crc_calc != crc_received:
        print(f"   ⚠️ Error de CRC en respuesta. Esperado: {hex(crc_calc)}, Recibido: {hex(crc_received)}")
        return None
        
    return {'cmd': cmd, 'arg': arg, 'payload': bytes(payload)}

def rescue_radio(port, bin_path):
    if not os.path.exists(bin_path):
        print(f"❌ Error: No se encuentra el archivo de firmware en '{bin_path}'")
        return False
        
    print(f"📖 Cargando binario de rescate: {bin_path}")
    with open(bin_path, 'rb') as f:
        firmware = f.read()
        
    firmware_len = len(firmware)
    print(f"✅ Firmware cargado. Tamaño original: {firmware_len} bytes ({firmware_len/1024:.2f} KB)")
    
    # Pad firmware to nearest 1024 bytes (with 0xFF or 0x00, let's use 0x00 to match official updater padding)
    if firmware_len % 1024 != 0:
        padding = 1024 - (firmware_len % 1024)
        firmware += b'\x00' * padding
        firmware_len = len(firmware)
        print(f"ℹ️ Firmware alineado a bloques de 1024 bytes. Nuevo tamaño: {firmware_len} bytes")
        
    total_blocks = firmware_len // 1024
    print(f"📦 Total de bloques a flashear: {total_blocks}")
    
    print(f"\n🔌 Abriendo puerto {port} a 115200 baudios...")
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
    except Exception as e:
        print(f"❌ Error al abrir puerto serie: {e}")
        return False
        
    # STEP 1: Send official BOOTLOADER handshake packet
    print("\n📡 [PASO 1] Enviando saludo oficial 'BOOTLOADER'...")
    bootloader_packet = pack_k6_packet(0x01, 0, b"BOOTLOADER")
    print(f"   Frame enviado (Hex): {bootloader_packet.hex()}")
    
    ser.write(bootloader_packet)
    ser.flush()
    
    # Wait for response
    resp = read_k6_response(ser)
    if not resp:
        print("❌ Error: La radio no responde al comando BOOTLOADER. Asegúrate de estar en modo UPDATE (pantalla encendida o LED parpadeando).")
        ser.close()
        return False
        
    print(f"   🎉 ¡Respuesta recibida! CMD={hex(resp['cmd'])}, ARG={hex(resp['arg'])}, Payload={resp['payload'].hex()}")
    
    # STEP 2: Send official engage CMD 0x04 ('G')
    print("\n📡 [PASO 2] Enviando comando de enganche de Flash ('G')...")
    engage_packet = pack_k6_packet(0x04, 0, b"G")
    print(f"   Frame enviado (Hex): {engage_packet.hex()}")
    
    ser.write(engage_packet)
    ser.flush()
    
    # Wait for response
    resp = read_k6_response(ser)
    if not resp:
        print("❌ Error: La radio no responde al comando de enganche de Flash.")
        ser.close()
        return False
        
    print(f"   🎉 ¡Enganche de Flash aceptado! CMD={hex(resp['cmd'])}, ARG={hex(resp['arg'])}")
    
    # STEP 3: Flash loop using CMD 0x03
    print("\n🚀 [PASO 3] Iniciando flasheo de firmware por bloques...")
    block_size = 1024
    
    for block_idx in range(total_blocks):
        offset = block_idx * block_size
        chunk = firmware[offset : offset + block_size]
        
        # Send block write
        block_packet = pack_k6_packet(0x03, block_idx, chunk)
        ser.write(block_packet)
        ser.flush()
        
        # Read confirmation (ACK packet has ARG = 0x06)
        resp = read_k6_response(ser, timeout=2.0)
        if not resp or resp['arg'] != 0x06:
            print(f"\n❌ Error al escribir el bloque {block_idx} en 0x{offset:04x}")
            if resp:
                print(f"   Respuesta inesperada: CMD={hex(resp['cmd'])}, ARG={hex(resp['arg'])}")
            ser.close()
            return False
            
        # Draw dynamic progress bar
        percent = (block_idx + 1) * 100 // total_blocks
        bar_len = 40
        filled_len = int(bar_len * (block_idx + 1) // total_blocks)
        bar = '█' * filled_len + '-' * (bar_len - filled_len)
        sys.stdout.write(f"\r⚡ Flasheando: [{bar}] {percent}% | Bloque {block_idx+1}/{total_blocks} (0x{offset:04x})")
        sys.stdout.flush()
        
    print("\n\n🎉 ¡ESCRITURA DE FIRMWARE COMPLETADA CON ÉXITO! 🎉")
    print("🔌 Cerrando puerto serie...")
    ser.close()
    
    print("\n✅ ¡Radio rescatada! Debería reiniciarse automáticamente con el nuevo firmware.")
    return True

if __name__ == '__main__':
    print("==================================================")
    print("      HERRAMIENTA DE RESCATE BAOFENG UV-K6        ")
    print("==================================================")
    
    port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]
    
    print(f"🔌 Puerto serial detectado: {port}\n")
    
    bin_dir = 'out/'
    original_path = os.path.join(bin_dir, 'original_factory.bin')
    
    # Find custom binaries starting with bfk6-
    custom_files = [f for f in os.listdir(bin_dir) if f.startswith('bfk6-') and f.endswith('.bin')]
    custom_files.sort()
    
    options = {}
    idx = 1
    
    if os.path.exists(original_path):
        options[idx] = ("Firmware Original de Fábrica (Limpio)", original_path)
        idx += 1
    else:
        print("⚠️ Advertencia: No se encontró 'out/original_factory.bin'")
        
    if custom_files:
        for f in custom_files:
            cpath = os.path.join(bin_dir, f)
            options[idx] = (f"Firmware Custom Half-Life: {f}", cpath)
            idx += 1
    else:
        print("⚠️ Advertencia: No se encontró ningún firmware custom 'bfk6-*.bin' en 'out/'")
        
    if not options:
        print("❌ Error: No se encontró ningún archivo de firmware (.bin) en la carpeta 'out/'")
        sys.exit(1)
        
    print("Selecciona el firmware que deseas flashear:")
    for o_idx, (name, path) in options.items():
        print(f"  [{o_idx}] {name}")
    print(f"  [{idx}] Salir")
    
    try:
        selection = input(f"\n👉 Ingresa tu opción (1-{idx}): ").strip()
        if not selection:
            print("❌ Operación cancelada.")
            sys.exit(0)
            
        sel_idx = int(selection)
        if sel_idx == idx:
            print("👋 Saliendo de la herramienta de rescate. ¡Hasta luego!")
            sys.exit(0)
            
        if sel_idx not in options:
            print("❌ Opción inválida.")
            sys.exit(1)
            
        selected_name, selected_path = options[sel_idx]
        
    except ValueError:
        print("❌ Entrada inválida. Debe ser un número.")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n👋 Operación cancelada por el usuario.")
        sys.exit(0)
        
    print("\n" + "="*50)
    print(f" Iniciando proceso para: {selected_name}")
    print("="*50 + "\n")
    
    success = rescue_radio(port, selected_path)
    if success:
        print("==================================================")
        print("            ¡PROCESO COMPLETADO CON ÉXITO!        ")
        print("==================================================")
    else:
        sys.exit(1)
