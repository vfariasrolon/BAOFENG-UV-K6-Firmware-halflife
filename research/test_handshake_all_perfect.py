import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Comprehensive list of candidates to try with absolute precision
candidates = [
    b"BF-K6PRO",            # Official string from EXE (8 bytes)
    b"BF-K6A  ",            # BF-K6A space padded (8 bytes)
    b"BF-K6A\x00\x00",      # BF-K6A null padded (8 bytes)
    b"BF-K6A",              # BF-K6A unpadded (6 bytes)
    b"BF-K6   ",            # BF-K6 space padded (8 bytes)
    b"BF-K6\x00\x00\x00",   # BF-K6 null padded (8 bytes)
    b"BF-K6",               # BF-K6 unpadded (5 bytes)
    b"BFK6_V02",            # From project title (8 bytes)
    b"BFNORMAL",            # Classic Baofeng (8 bytes)
    b"BFK6A",               # BFK6A unpadded (5 bytes)
    b"BFK6A  ",             # BFK6A space padded (7 bytes)
    b"BFK6A\x00\x00\x00",   # BFK6A null padded (8 bytes)
]

print("=== PROBADOR MAESTRO DE TODAS LAS PERMUTACIONES DE FIRMA ===")
print("Este script prueba todas las combinaciones posibles de modelo usando la secuencia física perfecta.\n")

for idx, cand in enumerate(candidates):
    print(f"\n[Prueba {idx+1}/{len(candidates)}] Probando: {cand!r} (Largo del modelo: {len(cand)} bytes)")
    
    try:
        # 1. Open serial port FIRST to absorb glitches
        ser = serial.Serial(port, 115200, timeout=1.0)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        print("🚨 SECUENCIA DE ENCENDIDO:")
        print("  1. Asegúrate de que la radio esté APAGADA.")
        print("  2. Mantén presionados los DOS botones naranjas laterales.")
        print("  3. Enciende la radio (Pantalla debe mostrar UPDATE).")
        input("👉 Presiona ENTER 2 segundos DESPUÉS de haber encendido la radio...")
        
        # Clear any power-on noise
        ser.flushInput()
        
        # Send exact handshake
        handshake = b"PROGRAM" + cand + b"\x55"
        print(f"📡 Enviando ráfaga ({len(handshake)} bytes): {handshake.hex()}")
        ser.write(handshake)
        ser.flush()
        
        # Read response
        time.sleep(0.2)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"🎉 Respuesta de la radio (Hex): {resp.hex()}")
            
            if 0x06 in resp:
                print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ MATCH ENCONTRADO! FIRMA CORRECTA: {cand!r} ⭐⭐")
                print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                
                print("📡 Enviando UPDATE...")
                ser.write(b"UPDATE")
                ser.flush()
                time.sleep(0.2)
                resp2 = ser.read(ser.in_waiting)
                print(f"🎉 Respuesta a UPDATE: {resp2.hex()}")
                if 0x06 in resp2:
                    print("   ✅ ACK recibido para UPDATE. ¡Bootloader listo para flasheo activo!")
                ser.close()
                sys.exit(0)
            else:
                # Print parsed error
                i = 0
                while i < len(resp):
                    if resp[i] == 0xAA and i + 8 <= len(resp):
                        pkt = resp[i:i+8]
                        print(f"   📦 Paquete Baofeng: CMD={hex(pkt[1])}, ARG={hex(pkt[2])}")
                        i += 8
                    else:
                        i += 1
        else:
            print("❌ Sin respuesta de la radio.")
            
        ser.close()
    except Exception as e:
        print(f"❌ Error de puerto: {e}")

print("\n❌ Se terminaron todos los candidatos sin éxito.")
sys.exit(1)
