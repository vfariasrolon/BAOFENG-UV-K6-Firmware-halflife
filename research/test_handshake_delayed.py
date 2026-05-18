import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

candidates = [
    b"BF-K6PRO",
    b"BF-K6A  ",
    b"BF-K6A\x00\x00",
    b"BF-K6A",
]

print("=== PROBADOR DE HANDSHAKE CON RETARDOS INTER-BYTES ===")
print("Este script introduce retardos de 10ms entre las partes del saludo para emular la planificación del sistema operativo Windows.\n")

for cand in candidates:
    print(f"\n👉 Probando candidato: {cand!r}")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        ser.dtr = True
        ser.rts = True
        ser.flushInput()
        ser.flushOutput()
        
        print("🚨 SECUENCIA DE ENCENDIDO:")
        print("  1. Asegúrate de que la radio esté APAGADA.")
        print("  2. Mantén presionados los DOS botones naranjas.")
        print("  3. Enciende la radio (UPDATE).")
        input("👉 Presiona ENTER 2 segundos DESPUÉS de haber encendido...")
        
        ser.flushInput()
        
        # Write parts with 10ms delays
        print("📡 Enviando 'PROGRAM'...")
        ser.write(b"PROGRAM")
        ser.flush()
        time.sleep(0.01)
        
        print(f"📡 Enviando modelo: {cand.hex()}...")
        ser.write(cand)
        ser.flush()
        time.sleep(0.01)
        
        print("📡 Enviando byte de control 0x55...")
        ser.write(b"\x55")
        ser.flush()
        
        # Read response
        time.sleep(0.2)
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"🎉 Respuesta de la radio (Hex): {resp.hex()}")
            
            if 0x06 in resp:
                print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                print(f"   ⭐⭐ SUCCESS! ¡EL BOOTLOADER ACEPTÓ EL HANDSHAKE! ⭐⭐")
                print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
                
                print("📡 Enviando UPDATE...")
                ser.write(b"UPDATE")
                ser.flush()
                time.sleep(0.2)
                resp2 = ser.read(ser.in_waiting)
                print(f"🎉 Respuesta a UPDATE: {resp2.hex()}")
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
            print("❌ Sin respuesta.")
            
        ser.close()
    except Exception as e:
        print(f"❌ Error: {e}")
        
sys.exit(1)
