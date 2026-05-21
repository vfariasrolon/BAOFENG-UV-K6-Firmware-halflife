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
    """Packs a command into the Baofeng UV-K6x protocol frame:
    [0xAA, CMD, ARG, LEN_MSB, LEN_LSB, ...DATA..., CRC_MSB, CRC_LSB, 0xEF]
    """
    length = len(payload)
    header = bytes([cmd, arg, (length >> 8) & 0xFF, length & 0xFF]) + payload
    crc = crc16_ccitt(header)
    packet = bytes([0xAA]) + header + bytes([(crc >> 8) & 0xFF, crc & 0xFF, 0xEF])
    return packet

def read_k6_response(ser, timeout=1.0):
    """Reads a response package from the radio and verifies its CRC and structure."""
    start = time.time()
    # Read header (first 5 bytes): [0xAA, CMD, ARG, LEN_MSB, LEN_LSB]
    header = bytearray()
    while time.time() - start < timeout:
        if ser.in_waiting >= 5:
            header = ser.read(5)
            break
        time.sleep(0.005)
        
    if len(header) < 5 or header[0] != 0xAA:
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
        return None
        
    payload = body[:payload_len]
    crc_received = (body[payload_len] << 8) | body[payload_len + 1]
    footer = body[payload_len + 2]
    
    if footer != 0xEF:
        return None
        
    # Verify CRC on CMD, ARG, LEN, and DATA
    header_for_crc = bytes(header[1:]) + bytes(payload)
    crc_calc = crc16_ccitt(header_for_crc)
    if crc_calc != crc_received:
        return None
        
    return {'cmd': cmd, 'arg': arg, 'payload': bytes(payload)}

def flash_firmware(port, bin_path):
    if not os.path.exists(bin_path):
        print(f"❌ Error: No se encuentra el archivo binario en {bin_path}")
        return False
        
    print(f"📖 Leyendo firmware: {bin_path}")
    with open(bin_path, 'rb') as f:
        firmware = f.read()
        
    firmware_len = len(firmware)
    print(f"✅ Firmware cargado. Tamaño: {firmware_len} bytes ({firmware_len/1024:.2f} KB)")
    
    # Pad to block size of 1024 bytes (with 0x00, matching bootloader logic)
    if firmware_len % 1024 != 0:
        padding = 1024 - (firmware_len % 1024)
        firmware += b'\x00' * padding
        firmware_len = len(firmware)
        print(f"ℹ️ Firmware alineado a bloques de 1024 bytes. Nuevo tamaño: {firmware_len} bytes")

    # Connect initially at 115200 baud
    print(f"\n🔌 Abriendo puerto serie {port} a 115200 baudios...")
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
    except Exception as e:
        print(f"❌ Error al abrir el puerto: {e}")
        return False
        
    # Step 1: Send handshake sequence ("PROGRAM" + "BF-K6   " + 0x55)
    print("📡 Iniciando protocolo de saludo pasivo...")
    handshake_bytes = b"PROGRAM" + b"BF-K6   " + b"\x55"
    
    # Try multiple times in case of timing/sync issues
    connected = False
    for attempt in range(1, 6):
        print(f"   Intento {attempt}/5: Enviando ráfaga de conexión...")
        ser.write(handshake_bytes)
        ser.flush()
        
        # Wait for 0x06 ACK from radio
        time.sleep(0.05)
        if ser.in_waiting > 0:
            resp = ser.read(1)
            if len(resp) > 0 and resp[0] == 0x06:
                print("✅ Saludo 1 aceptado (ACK recibido).")
                connected = True
                break
        time.sleep(0.2)
        
    if not connected:
        print("❌ Error: La radio no responde al saludo. Verifica que esté en modo 'UPDATE' y el cable conectado.")
        ser.close()
        return False
        
    # Step 2: Send "UPDATE" confirmation
    print("📡 Enviando confirmación de modo UPDATE...")
    ser.write(b"UPDATE")
    ser.flush()
    
    # Wait for 0x06 ACK from radio
    resp = ser.read(1)
    if not resp or resp[0] != 0x06:
        print("❌ Error: Sin respuesta a la confirmación de UPDATE.")
        ser.close()
        return False
    print("✅ Confirmación UPDATE aceptada (ACK recibido).")
    
    # Step 3: Re-open port at 115200 baud (to match original C# driver sequence)
    print("🔌 Reiniciando puerto serie (115200 baudios)...")
    ser.close()
    time.sleep(0.05)
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
    except Exception as e:
        print(f"❌ Error al reabrir el puerto: {e}")
        return False
        
    time.sleep(0.02) # Sleep 20ms like C# Thread.Sleep(20)
    
    # Step 4: Active protocol start (CMD_GETTYPE / 0x42)
    print("📡 Sincronizando con el bootloader activo...")
    ser.write(pack_k6_packet(0x42, 0))
    ser.flush()
    
    resp = read_k6_response(ser, timeout=1.0)
    if not resp or resp['cmd'] != 0x06:
        print("❌ Error: La radio no respondió a la sincronización del bootloader.")
        ser.close()
        return False
    print("✅ Bootloader sincronizado y listo para recibir comandos.")
    
    # Step 5: CMD_ERASE (0x01) with "erase" payload
    print("\n🧹 Enviando comando de borrado de memoria Flash...")
    ser.write(pack_k6_packet(0x01, 0, b"erase"))
    ser.flush()
    
    # Erase operation takes longer, use 3 seconds timeout
    resp = read_k6_response(ser, timeout=3.0)
    if not resp or resp['cmd'] != 0x06:
        print("❌ Error: Falló la operación de borrado de flash.")
        ser.close()
        return False
    print("✅ Memoria borrada con éxito.")
    
    # Step 6: Flash blocks loop (CMD_WRITE = 0x03)
    print("\n🚀 Iniciando escritura en memoria Flash...")
    block_size = 1024
    total_blocks = firmware_len // block_size
    
    for block_idx in range(total_blocks):
        offset = block_idx * block_size
        chunk = firmware[offset : offset + block_size]
        
        # Send block command
        ser.write(pack_k6_packet(0x03, block_idx, chunk))
        ser.flush()
        
        # Wait for block confirmation ACK (0x06)
        resp = read_k6_response(ser, timeout=1.5)
        if not resp or resp['cmd'] != 0x06:
            print(f"\n❌ Error al escribir el bloque {block_idx} en el desplazamiento 0x{offset:04x}")
            ser.close()
            return False
            
        # Draw elegant progress bar
        percent = (block_idx + 1) * 100 // total_blocks
        bar_len = 40
        filled_len = int(bar_len * (block_idx + 1) // total_blocks)
        bar = '█' * filled_len + '-' * (bar_len - filled_len)
        sys.stdout.write(f"\r⚡ Flasheando: [{bar}] {percent}% | Bloque {block_idx+1}/{total_blocks} (0x{offset:04x})")
        sys.stdout.flush()
        
    # Step 7: CMD_END (0x45)
    print("\n\n🎉 ¡Escritura de firmware completada! Finalizando...")
    ser.write(pack_k6_packet(0x45, 0))
    ser.flush()
    
    resp = read_k6_response(ser, timeout=1.0)
    if not resp or resp['cmd'] != 0x06:
        print("⚠️ Advertencia: La radio no confirmó la finalización final (CMD_END).")
    else:
        print("✅ Operación finalizada con éxito.")
        
    print("\n🎉 ¡PROCESO FINALIZADO CON ÉXITO! Tu Baofeng UV-K6 debería reiniciarse sola. 🎉")
    ser.close()
    return True

if __name__ == '__main__':
    print("==================================================")
    print("  NATIVE LINUX FLASHER FOR BAOFENG UV-K6x SERIES  ")
    print("==================================================")
    
    # Default parameters (Support environment variables)
    port = os.environ.get('PORT', '/dev/ttyUSB0')
    
    # Find most recent binary in out/ directory
    bin_dir = 'out/'
    bin_files = [f for f in os.listdir(bin_dir) if f.startswith('bfk6-') and f.endswith('.bin')]
    if not bin_files:
        print("❌ Error: No se encontró ningún archivo de firmware compilado en la carpeta 'out/'")
        sys.exit(1)
        
    bin_files.sort(reverse=True)
    bin_path = os.path.join(bin_dir, bin_files[0])
    
    if len(sys.argv) > 1:
        port = sys.argv[1]
        
    success = flash_firmware(port, bin_path)
    if success:
        print("==================================================")
        print("           ¡DESPLIEGUE FINALIZADO!                ")
        print("==================================================")
    else:
        sys.exit(1)
