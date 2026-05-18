import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Candidates for the 8-byte model string
candidates = [
    b"BFNORMAL",
    b"BF-K6PRO",
    b"BF-K6A  ",
    b"BF-K6A\x00\x00",
    b"BF-K6   ",
    b"BF-K6\x00\x00\x00\x00",
    b"BF-K5   ",
    b"BF-K5\x00\x00\x00\x00",
    b"BF-K5PRO",
    b"BF-K5A  ",
    b"BF-K5A\x00\x00",
    b"BFK6A   ",
    b"BFK6A\x00\x00\x00",
    b"BF-K6A-A",
    b"BF-K6A-B",
    b"BF-K6AV2",
    b"BF-K6AV1",
    b"BF-K6A-V",
    b"BF-K6A_V",
    b"BF-K6A V",
    b"BF-K6A V1",
    b"BF-K6A V2",
    b"BF-K6A_V2",
    b"BF-K6A_V1",
    b"BFK6    ",
    b"BFK6\x00\x00\x00\x00",
]

print("=== ESCÁNER DE MODELOS CON SECUENCIA DE ACTIVACIÓN COMPLETA ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

for m in candidates:
    # Ensure exactly 8 bytes
    if len(m) < 8:
        m = m.ljust(8, b' ')
    elif len(m) > 8:
        m = m[:8]
        
    handshake_bytes = b"PROGRAM" + m + b"\x55"
    print(f"📡 Probando modelo: {m!r} (Handshake: {handshake_bytes.hex()})")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        time.sleep(0.05) # 전기적 안정화
        
        # Step 1: Send unary activation byte 0x55
        ser.write(b"\x55")
        ser.flush()
        
        # Grace delay of 50ms
        time.sleep(0.05)
        
        # Step 2: Send identification handshake
        ser.write(handshake_bytes)
        ser.flush()
        
        # Wait up to 150ms for response
        time.sleep(0.15)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"   🎉 Respuesta: {resp.hex()}")
            
            # Look for 0x06 (ACK)
            if 0x06 in resp:
                print("   ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ MATCH! MODELO CORRECTO ENCONTRADO: {m!r} ⭐⭐")
                print("   ⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                ser.close()
                sys.exit(0)
            
            # Analyze packets
            i = 0
            while i < len(resp):
                if resp[i] == 0xAA:
                    if i + 8 <= len(resp):
                        pkt = resp[i:i+8]
                        cmd = pkt[1]
                        arg = pkt[2]
                        print(f"   📦 Paquete Baofeng: CMD={hex(cmd)}, ARG={hex(arg)}")
                        if arg == 0xE1:
                            print("      ❌ E1 = Version/Handshake code error (Firma rechazada)")
                        i += 8
                    else:
                        i += 1
                else:
                    i += 1
        else:
            print("   📭 Sin respuesta")
            
        ser.close()
    except Exception as e:
        print(f"   ❌ Error: {e}")
    time.sleep(0.1) # Small gap between iterations
    print("-" * 60)

print("\n❌ Ninguna firma de modelo fue aceptada por la radio.")
sys.exit(1)
