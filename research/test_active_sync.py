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

def pack_k6_packet(cmd: int, arg: int, payload: bytes = b'') -> bytes:
    length = len(payload)
    header = bytes([cmd, arg, (length >> 8) & 0xFF, length & 0xFF]) + payload
    crc = crc16_ccitt(header)
    packet = bytes([0xAA]) + header + bytes([(crc >> 8) & 0xFF, crc & 0xFF, 0xEF])
    return packet

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

print("=== PROBADOR DE SINCRONIZACIÓN ACTIVA DIRECTA (CMD_GETTYPE) ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

try:
    ser = serial.Serial(port, 115200, timeout=0.5)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    # Delay for electrical stabilization
    time.sleep(0.1)
    
    # CMD_GETTYPE is 0x42
    packet = pack_k6_packet(0x42, 0)
    print(f"📡 Enviando CMD_GETTYPE (0x42): {packet.hex()}")
    ser.write(packet)
    ser.flush()
    
    time.sleep(0.2)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"   🎉 ¡RESPUESTA RECIBIDA! Bytes: {resp.hex()}")
        if len(resp) >= 8 and resp[0] == 0xAA:
            print(f"   📦 Paquete recibido con cabecera 0xAA!")
            # Decapsulate
            cmd = resp[1]
            arg = resp[2]
            payload_len = (resp[3] << 8) | resp[4]
            payload = resp[5:5+payload_len]
            print(f"      CMD: {hex(cmd)}")
            print(f"      ARG: {hex(arg)}")
            print(f"      Longitud Payload: {payload_len}")
            print(f"      Payload: {payload} (Hex: {payload.hex()})")
    else:
        print("   📭 Sin respuesta de la radio")
        
    ser.close()
except Exception as e:
    print(f"   ❌ Error: {e}")
