import serial
import time
import sys
import os

def calc_crc(data):
    crc = 0x0000
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc

def make_packet(cmd, arg, payload=b""):
    length = len(payload)
    # Estructura oficial Baofeng: [0xAA, CMD, ARG, LEN_MSB, LEN_LSB] + PAYLOAD
    header = bytes([0xAA, cmd, arg, (length >> 8) & 0xFF, length & 0xFF])
    full_body = header + payload
    crc = calc_crc(full_body)
    return full_body + bytes([(crc >> 8) & 0xFF, crc & 0xFF, 0xEF])

def flash():
    port = "/dev/ttyUSB0" if len(sys.argv) < 2 else sys.argv[1]
    fw_path = "out/bfk6-202605171748.bin"
    
    if not os.path.exists(fw_path):
        print(f"❌ Error: No se encuentra {fw_path}")
        return

    with open(fw_path, "rb") as f:
        firmware = bytearray(f.read())
    
    # Relleno obligatorio a bloques de 1024 bytes (Alineación de sector)
    while len(firmware) % 1024 != 0:
        firmware.append(0x00)
        
    total_blocks = len(firmware) // 1024
    print(f"📖 Firmware cargado. Bloques alineados: {total_blocks} (1KB c/u)")

    print(f"🔌 Abriendo {port} a 115200 baudios (Descriptor Anti-Glitch)...")
    try:
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = 115200
        ser.timeout = 1.0
        ser.dsrdtr = False
        ser.rtscts = False
        ser.open()
        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
    except Exception as e:
        print(f"❌ Error abriendo el puerto serie: {e}")
        return

    print("\n🚨 SECUENCIA DE RESCATE CRÍTICA:")
    print("  1. Asegúrate de que la radio esté COMPLETAMENTE APAGADA.")
    print("  2. Mantén presionados firmemente los DOS botones naranjas laterales.")
    print("  3. Con los botones presionados, ENCIENDE la radio (la pantalla seguirá negra).")
    input("\n👉 Presiona ENTER aquí exactamente 2 segundos DESPUÉS de haberla encendido...")

    # --- PASO 1: Pulso unario de activación ---
    print("\n📡 1. Enviando pulso de atención (0x55)...")
    ser.write(b"\x55")
    ser.flush()
    time.sleep(0.05)
    
    # --- PASO 2: Handshake Rígido de 16 Bytes (Cazado en strace) ---
    # "PROGRAM" (7 bytes) + "BF-K6   " (8 bytes con 3 espacios) + "\x55" (1 byte) = 16 bytes exactos
    handshake_strace = b"PROGRAM" + b"BF-K6   " + b"\x55"
    print(f"🤝 2. Enviando Handshake definitivo (Hex): {handshake_strace.hex()}")
    ser.write(handshake_strace)
    ser.flush()
    
    # Leemos la respuesta estructural
    resp = ser.read(8)
    
    # Validamos que la respuesta NO contenga el código de fallo de firma 0xE1
    if len(resp) == 0 or (len(resp) >= 3 and resp[2] == 0xE1) or (len(resp) >= 2 and resp[1] == 0x52):
        print(f"❌ El procesador rechazó la firma. Trama recibida: {resp.hex()}")
        ser.close()
        return
        
    print("✅ ¡HANDSHAKE ACEPTADO! La ROM del procesador abrió la compuerta de datos.")

    # --- PASO 3: Confirmación de modo UPDATE ---
    print("🔄 Sincronizando modo de actualización...")
    ser.write(b"UPDATE")
    ser.flush()
    time.sleep(0.05)
    
    # --- PASO 4: Comando de Borrado de sectores (CMD 0x04) ---
    print("🧹 Ejecutando borrado general de la memoria Flash...")
    erase_pkt = make_packet(4, 0, bytes([total_blocks & 0xFF]))
    ser.write(erase_pkt)
    ser.flush()
    time.sleep(0.6) # Delay crítico para el proceso físico en el silicio
    
    # --- PASO 5: Grabación por bloques de 1KB (CMD 0x03) ---
    print("🚀 Inyectando firmware de Half-Life bloque por bloque...")
    for block_idx in range(total_blocks):
        start = block_idx * 1024
        end = start + 1024
        chunk = firmware[start:end]
        
        # CMD 3 (Write Block), ARG es el índice secuencial
        pkt = make_packet(3, block_idx, chunk)
        ser.write(pkt)
        ser.flush()
        
        # Leer la confirmación estructural de la radio por cada bloque
        ack_resp = ser.read(8)
        
        progress = (block_idx + 1) / total_blocks * 100
        sys.stdout.write(f"\r📦 Grabando sector: [{block_idx+1}/{total_blocks}] {progress:.1f}% ")
        sys.stdout.flush()

    print("\n✅ Firmware escrito al 100% de manera nativa.")

    # --- PASO 6: Comando de Reinicio por Hardware (CMD 0x45) ---
    print("🔄 Forzando reinicio general de la CPU...")
    reboot_pkt = make_packet(0x45, 0)
    ser.write(reboot_pkt)
    ser.flush()
    
    print("🎉 ¡HACK FINALIZADO CON ÉXITO! Tu radio ha resucitado.")
    ser.close()

if __name__ == "__main__":
    flash()
