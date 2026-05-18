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

print("=== PROBADOR DE PROTOCOLO PASIVO COMPLETO DE BAOFENG UV-K6 ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

try:
    # 1. Open port at 115200
    ser = serial.Serial(port, 115200, timeout=0.5)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    time.sleep(0.1) # Stabilization
    
    # Paso 1: Pulso de Activación Unario (0x55)
    print("📡 Paso 1: Enviando pulso de activación unario (0x55)...")
    ser.write(b"\x55")
    ser.flush()
    
    # Delay de gracia de 50ms
    time.sleep(0.05)
    
    # Paso 2: Handshake de Identificación KDH (b"PROGRAMBF-K6PRO\x55")
    handshake_bytes = b"PROGRAM" + b"BF-K6PRO" + b"\x55"
    print(f"📡 Paso 2: Enviando saludo de identificación: {handshake_bytes.hex()}")
    ser.write(handshake_bytes)
    ser.flush()
    
    # Wait for 0x06 ACK
    time.sleep(0.1)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"   🎉 Respuesta Paso 2: {resp.hex()} | {resp}")
        if 0x06 in resp:
            print("   ✅ ACK (0x06) recibido con éxito en Paso 2!")
            
            # Paso 3: Confirmación de Estado (b"UPDATE")
            print("📡 Paso 3: Enviando confirmación b'UPDATE'...")
            ser.write(b"UPDATE")
            ser.flush()
            
            # Wait for 0x06 ACK
            time.sleep(0.1)
            if ser.in_waiting > 0:
                resp2 = ser.read(ser.in_waiting)
                print(f"   🎉 Respuesta Paso 3: {resp2.hex()} | {resp2}")
                if 0x06 in resp2:
                    print("   ✅ ACK (0x06) recibido con éxito en Paso 3!")
                    print("   ⭐⭐ ¡SALUDO INICIAL COMPLETADO CON ÉXITO! ⭐⭐")
                    
                    # Try active synchronization (CMD_GETTYPE)
                    print("\n📡 Intentando sincronización activa empaquetada (CMD_GETTYPE)...")
                    packet = pack_k6_packet(0x42, 0)
                    ser.write(packet)
                    ser.flush()
                    
                    time.sleep(0.1)
                    if ser.in_waiting > 0:
                        resp3 = ser.read(ser.in_waiting)
                        print(f"   🎉 Respuesta a CMD_GETTYPE: {resp3.hex()}")
                    else:
                        print("   📭 Sin respuesta a CMD_GETTYPE")
                else:
                    print("   ❌ Error: No se recibió ACK para UPDATE")
            else:
                print("   📭 Sin respuesta para UPDATE")
        else:
            print("   ❌ Error: No se recibió ACK (0x06) para el saludo de identificación")
    else:
        print("   📭 Sin respuesta para el saludo de identificación")
        
    ser.close()
except Exception as e:
    print(f"   ❌ Error de comunicación: {e}")
