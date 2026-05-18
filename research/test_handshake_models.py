import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# List of 8-byte model strings to test
models = [
    b"BFNORMAL",
    b"BF-K6PRO",
    b"BF-K6A\x00\x00\x00",
    b"BF-K6A  ",
    b"BF-K6\x00\x00\x00\x00",
    b"BF-K6   ",
    b"BF-K6B\x00\x00\x00",
    b"BF-K6B  ",
    b"BF-K6C\x00\x00\x00",
    b"BF-K6C  ",
    b"BF-K5\x00\x00\x00\x00",
    b"BF-K5   ",
    b"BF-K5PRO",
    b"BF-K5A\x00\x00\x00",
    b"BF-K5A  ",
]

print("=== PROBADOR DE MODELOS DE HANDSHAKE PARA BAOFENG UV-K6 ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

# Use DTR/RTS False or True? Standard C# sets them to True, let's keep them True
dtr_state = True
rts_state = True

for m in models:
    # Ensure it is exactly 8 bytes
    if len(m) < 8:
        m = m.ljust(8, b' ')
    elif len(m) > 8:
        m = m[:8]
        
    handshake_bytes = b"PROGRAM" + m + b"\x55"
    print(f"📡 Probando modelo: {m!r} (Handshake: {handshake_bytes.hex()})")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = dtr_state
        ser.rts = rts_state
        ser.flushInput()
        ser.flushOutput()
        
        # Stabilization delay
        time.sleep(0.1)
        
        ser.write(handshake_bytes)
        ser.flush()
        
        # Wait up to 300ms for response
        time.sleep(0.1)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"   🎉 Respuesta: {resp.hex()}")
            
            # Analyze response
            if len(resp) >= 8 and resp[0] == 0xAA and resp[-1] == 0xEF:
                # Baofeng packed packet
                cmd = resp[1]
                arg = resp[2]
                print(f"   📦 Paquete Baofeng: CMD={hex(cmd)}, ARG={hex(arg)}")
                if arg == 0xE1:
                    print("   ❌ E1 = Version Error (Modelo no aceptado)")
                elif arg == 0xE2:
                    print("   ⭐⭐ E2 = SUCCESS / MATCH! ESTE ES EL MODELO GANADOR! ⭐⭐")
                    ser.close()
                    sys.exit(0)
                else:
                    print(f"   ❓ ARG={hex(arg)} (Respuesta inesperada)")
            elif len(resp) == 1 and resp[0] == 0x06:
                print("   ⭐⭐ 0x06 = SUCCESS / MATCH! (ACK de 1 byte) ⭐⭐")
                ser.close()
                sys.exit(0)
            else:
                print(f"   ❓ Respuesta no reconocida: {resp}")
        else:
            print("   📭 Sin respuesta")
            
        ser.close()
    except Exception as e:
        print(f"   ❌ Error: {e}")
    print("-" * 60)

print("\n❌ Ningún modelo dio éxito. Prueba a verificar si la radio está en modo UPDATE.")
sys.exit(1)
