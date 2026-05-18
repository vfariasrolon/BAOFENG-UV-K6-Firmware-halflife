import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Shortlist of short candidates without any padding
candidates = [
    b"BF-K6",     # 5 bytes
    b"BFK6",      # 4 bytes
    b"BFK6A",     # 5 bytes
    b"K6",        # 2 bytes
    b"UV-K6",     # 5 bytes
    b"UV-K6A",    # 6 bytes
    b"BF-K6PRO",  # 8 bytes (tried but let's list it for completeness)
]

print("=== ESCÁNER DE MODELOS CORTOS (SIN RELLENO) ===")
print("Este escáner prueba modelos con su largo exacto sin rellenar con espacios o nulos.")
print("Requiere reiniciar la radio entre pruebas.\n")

for idx, m in enumerate(candidates):
    print(f"\n[Candidato {idx+1}/{len(candidates)}]: {m!r} (Largo del modelo: {len(m)} bytes)")
    print("👉 PASOS A SEGUIR:")
    print("  1. Apaga la radio por completo.")
    print("  2. Enciéndela sosteniendo los dos botones naranjas (modo UPDATE).")
    input("  3. Presiona ENTER aquí para enviar la prueba...")
    
    handshake_bytes = b"PROGRAM" + m + b"\x55"
    print(f"📡 Enviando ráfaga ({len(handshake_bytes)} bytes): {handshake_bytes.hex()}")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        time.sleep(0.1)
        ser.write(handshake_bytes)
        ser.flush()
        
        time.sleep(0.2)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"   🎉 Respuesta: {resp.hex()}")
            if 0x06 in resp:
                print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ MATCH! MODELO CORRECTO ENCONTRADO: {m!r} ⭐⭐")
                print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                
                print("📡 Enviando UPDATE...")
                ser.write(b"UPDATE")
                ser.flush()
                time.sleep(0.2)
                resp2 = ser.read(ser.in_waiting)
                print(f"   🎉 Respuesta a UPDATE: {resp2.hex()}")
                if 0x06 in resp2:
                    print("   ✅ ACK recibido para UPDATE. ¡Bootloader listo para flasheo!")
                ser.close()
                sys.exit(0)
            else:
                i = 0
                while i < len(resp):
                    if resp[i] == 0xAA and i + 8 <= len(resp):
                        pkt = resp[i:i+8]
                        print(f"   📦 Paquete Baofeng: CMD={hex(pkt[1])}, ARG={hex(pkt[2])}")
                        i += 8
                    else:
                        i += 1
        else:
            print("   📭 Sin respuesta de la radio")
            
        ser.close()
    except Exception as e:
        print(f"   ❌ Error: {e}")
        
print("\n❌ Se terminaron los candidatos de la lista.")
sys.exit(1)
