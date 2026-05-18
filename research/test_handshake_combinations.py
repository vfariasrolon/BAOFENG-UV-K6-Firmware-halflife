import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]
handshake_bytes = b"PROGRAM" + b"BF-K6PRO" + b"\x55"

print("=== PROBADOR DE COMBINACIONES DE DTR / RTS PARA BAOFENG UV-K6 ===")
print("Asegúrate de que la radio esté en modo UPDATE (botones naranjas presionados al encender).")
print(f"Puerto: {port}\n")

combinations = [
    (False, False),
    (True, False),
    (False, True),
    (True, True)
]

for dtr, rts in combinations:
    print(f"🔌 Probando con DTR = {dtr}, RTS = {rts}...")
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = dtr
        ser.rts = rts
        ser.flushInput()
        ser.flushOutput()
        
        # Give a small delay for electrical stabilization
        time.sleep(0.1)
        
        # Try sending handshake 3 times
        for attempt in range(1, 4):
            print(f"   Intento {attempt}: Enviando saludo...")
            ser.write(handshake_bytes)
            ser.flush()
            
            time.sleep(0.1)
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                print(f"   🎉 ¡RESPUESTA RECIBIDA! Bytes: {resp.hex()} | Caracteres: {resp}")
                if 0x06 in resp:
                    print(f"   ⭐⭐ COMBINACIÓN GANADORA ENCONTRADA: DTR={dtr}, RTS={rts} ⭐⭐")
                    ser.close()
                    sys.exit(0)
            time.sleep(0.1)
            
        ser.close()
    except Exception as e:
        print(f"   ❌ Error: {e}")
    print("-" * 50)

print("\n❌ Ninguna combinación dio respuesta. Verifica el cable, puerto y que la pantalla diga UPDATE.")
sys.exit(1)
