import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Shortlist of highly likely 8-byte model strings for Baofeng UV-K6A
candidates = [
    b"BF-K6A  ",
    b"BF-K6A\x00\x00",
    b"BF-K6PRO",
    b"BFNORMAL",
    b"BF-K6   ",
    b"BF-K6\x00\x00\x00",
    b"BFK6A   ",
    b"BFK6A\x00\x00\x00",
]

print("=== ESCÁNER INTERACTIVO DE FIRMAS DE MODELO (PASIVO) ===")
print("Este escáner requiere reiniciar la radio entre pruebas para evitar bloqueos del microcontrolador.\n")

for idx, m in enumerate(candidates):
    print(f"\n[Candidato {idx+1}/{len(candidates)}]: {m!r}")
    print("👉 PASOS A SEGUIR:")
    print("  1. Apaga la radio por completo.")
    print("  2. Enciéndela sosteniendo los dos botones naranjas (debe mostrar UPDATE en pantalla).")
    input("  3. Presiona ENTER aquí para enviar la prueba...")
    
    # Ensure exactly 8 bytes
    if len(m) < 8:
        m = m.ljust(8, b' ')
    elif len(m) > 8:
        m = m[:8]
        
    handshake_bytes = b"PROGRAM" + m + b"\x55"
    print(f"📡 Enviando ráfaga: {handshake_bytes.hex()}")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        time.sleep(0.1)
        
        # Send handshake in a single burst
        ser.write(handshake_bytes)
        ser.flush()
        
        # Wait for response
        time.sleep(0.2)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"   🎉 Respuesta en bruto: {resp.hex()}")
            if 0x06 in resp:
                print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ MATCH! MODELO CORRECTO ENCONTRADO: {m!r} ⭐⭐")
                print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                
                # Try sending UPDATE confirmation
                print("📡 Enviando paso 3 (UPDATE)...")
                ser.write(b"UPDATE")
                ser.flush()
                time.sleep(0.2)
                resp2 = ser.read(ser.in_waiting)
                print(f"   🎉 Respuesta a UPDATE: {resp2.hex()}")
                if 0x06 in resp2:
                    print("   ✅ ACK recibido para UPDATE. ¡Bootloader listo para flasheo activo!")
                ser.close()
                sys.exit(0)
            else:
                # Decapsulate error
                i = 0
                while i < len(resp):
                    if resp[i] == 0xAA and i + 8 <= len(resp):
                        pkt = resp[i:i+8]
                        cmd = pkt[1]
                        arg = pkt[2]
                        print(f"   📦 Paquete Baofeng: CMD={hex(cmd)}, ARG={hex(arg)}")
                        if arg == 0xE1:
                            print("      ❌ E1 = Handshake/Version Error (Rechazado)")
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
