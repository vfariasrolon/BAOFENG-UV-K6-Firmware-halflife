import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Candidates to try in order
candidates = [
    b"BF-K6   ",
    b"BF-K6PRO",
    b"BFK6_V02",
    b"BF-K6A",
]

print("=== PROBADOR DE HANDSHAKE DE PRECISIÓN (EVITA GLITCHES DE APERTURA) ===")
print("Este script abre el puerto serie PRIMERO para evitar que los pulsos de inicialización bloqueen la radio.\n")

for cand in candidates:
    print(f"\n👉 Probando candidato: {cand!r}")
    
    try:
        # 1. Open serial port FIRST
        ser = serial.Serial(port, 115200, timeout=1.0)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        print(f"🔌 Puerto {port} abierto e inicializado con éxito.")
        print("🚨 PASO CRUCIAL:")
        print("  1. Asegúrate de que la radio esté APAGADA.")
        print("  2. Mantén presionados los DOS botones naranjas laterales.")
        print("  3. AHORA enciende la radio (debe mostrar UPDATE).")
        input("👉 Presiona ENTER aquí 2 segundos DESPUÉS de haber encendido la radio...")
        
        # Clear any glitch bytes received during power-on
        ser.flushInput()
        
        # Send handshake
        handshake = b"PROGRAM" + cand + b"\x55"
        print(f"📡 Enviando ráfaga: {handshake.hex()}")
        ser.write(handshake)
        ser.flush()
        
        # Read response
        time.sleep(0.2)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"🎉 Respuesta de la radio (Hex): {resp.hex()}")
            
            # Check for ACK
            if 0x06 in resp:
                print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ MATCH ENCONTRADO: {cand!r} ⭐⭐")
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
                # Analyze error packet
                i = 0
                while i < len(resp):
                    if resp[i] == 0xAA and i + 8 <= len(resp):
                        pkt = resp[i:i+8]
                        print(f"   📦 Paquete Baofeng: CMD={hex(pkt[1])}, ARG={hex(pkt[2])}")
                        if pkt[2] == 0xE1:
                            print("      ❌ E1 = Handshake/Version Error (Firma Rechazada)")
                        i += 8
                    else:
                        i += 1
        else:
            print("❌ Sin respuesta de la radio.")
            
        ser.close()
    except Exception as e:
        print(f"❌ Error de puerto: {e}")
        
print("\n❌ Se terminaron los candidatos.")
sys.exit(1)
