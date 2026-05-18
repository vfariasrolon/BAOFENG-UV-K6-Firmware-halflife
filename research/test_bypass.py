import serial
import time
import sys

def crc16_ccitt(data: bytes) -> int:
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

def make_packet(cmd: int, arg: int, payload: bytes = b'') -> bytes:
    length = len(payload)
    header = bytes([cmd, arg, (length >> 8) & 0xFF, length & 0xFF]) + payload
    crc = crc16_ccitt(header)
    packet = bytes([0xAA]) + header + bytes([(crc >> 8) & 0xFF, crc & 0xFF, 0xEF])
    return packet

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

print("=== PRUEBA DE BYPASS DIRECTO (CMD 0x42) ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

try:
    ser = serial.Serial(port, 115200, timeout=1.0)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    time.sleep(0.1)
    
    # --- PASO 1: El pulso de activación secreto extraído de HandShake_0 ---
    print("🎯 Enviando pulso de activación (0x55)...")
    ser.write(b"\x55")
    ser.flush()
    time.sleep(0.05)
    
    # --- PASO 2: Bypass de Handshake mediante CMD 0x42 (Sincronización Activa) ---
    print("🤝 Forzando sincronización binaria bypass (CMD 0x42)...")
    sync_packet = make_packet(0x42, 0, b"")
    ser.write(sync_packet)
    ser.flush()
    
    # Leemos la respuesta estructurada del radio (8 bytes)
    resp = ser.read(8)
    if len(resp) == 0:
        print("❌ El radio no respondió al paquete de sincronización binaria.")
    else:
        print(f"🎉 Respuesta del bootloader recibida en hexadecimal: {resp.hex()}")
        if len(resp) >= 3:
            if resp[1] == 0x52 and resp[2] == 0xE1:
                print("❌ El procesador sigue requiriendo la firma de modelo estricta.")
            else:
                print("✅ ¡Bypass exitoso! El bootloader aceptó la sincronización directa.")
        else:
            print("⚠️ Respuesta demasiado corta.")
            
    ser.close()
except Exception as e:
    print(f"❌ Error de puerto: {e}")
