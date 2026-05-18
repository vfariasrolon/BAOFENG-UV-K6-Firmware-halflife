import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

# Candidates to try: just BF-K6 with 3 spaces padding
cand = b"BF-K6   "

print("=== PROBADOR DE HANDSHAKE DE ESPACIOS (VALIDADOR DE FIRMA DEFINITIVA) ===")
print(f"👉 Probando firma definitiva: {cand!r} (Largo: {len(cand)} bytes)")

try:
    # 1. Open serial port FIRST
    ser = serial.Serial(port, 115200, timeout=1.0)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    print(f"🔌 Puerto {port} abierto e inicializado con éxito.")
    print("🚨 SECUENCIA DE PRUEBA:")
    print("  1. Apaga la radio por completo.")
    print("  2. Mantén presionados los DOS botones naranjas laterales.")
    print("  3. Enciende la radio (la pantalla debe mostrar UPDATE).")
    input("👉 Presiona ENTER aquí 2 segundos DESPUÉS de encender la radio...")
    
    # Clear any junk
    ser.flushInput()
    
    # Send handshake
    handshake = b"PROGRAM" + cand + b"\x55"
    print(f"📡 Enviando ráfaga (16 bytes): {handshake.hex()}")
    ser.write(handshake)
    ser.flush()
    
    # Read response
    time.sleep(0.2)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"🎉 Respuesta de la radio (Hex): {resp.hex()}")
        
        # Check for ACK (0x06)
        if 0x06 in resp:
            print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
            print("   ⭐⭐ ¡¡¡ÉXITO ROTUNDO!!! ⭐⭐")
            print(f"   Firma {cand!r} ACEPTADA por la radio.")
            print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
            
            print("📡 Enviando confirmación UPDATE...")
            ser.write(b"UPDATE")
            ser.flush()
            time.sleep(0.2)
            resp2 = ser.read(ser.in_waiting)
            print(f"🎉 Respuesta a UPDATE (Hex): {resp2.hex()}")
            if 0x06 in resp2:
                print("   ✅ ACK recibido para UPDATE. ¡El canal de comunicación está 100% abierto!")
        else:
            print("❌ La radio respondió pero RECHAZÓ la firma (sin ACK 0x06).")
            # Analyze response
            i = 0
            while i < len(resp):
                if resp[i] == 0xAA and i + 8 <= len(resp):
                    pkt = resp[i:i+8]
                    print(f"   📦 Paquete Baofeng: CMD={hex(pkt[1])}, ARG={hex(pkt[2])}")
                    i += 8
                else:
                    i += 1
    else:
        print("❌ Sin respuesta de la radio (verifica la conexión o el cable).")
        
    ser.close()
except Exception as e:
    print(f"❌ Error: {e}")
