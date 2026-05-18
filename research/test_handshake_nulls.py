import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

print("=== PRUEBA DE HANDSHAKE LIMPIO CON NULOS ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

try:
    ser = serial.Serial(port, 115200, timeout=1.0)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    time.sleep(0.1)
    
    # --- PASO 1: Pulso de activación ---
    print("🎯 Enviando pulso de activación (0x55)...")
    ser.write(b"\x55")
    ser.flush()
    time.sleep(0.05)
    
    # --- PASO 2: Handshake Base Puro (Sin basura dinámica) ---
    print("🤝 Probando Handshake Base Limpio...")
    handshake_puro = b"PROGRAM" + b"\x00" * 8 + b"\x55"
    print(f"Bytes enviados (Hex): {handshake_puro.hex()}")
    
    ser.write(handshake_puro)
    ser.flush()
    
    # Leemos la respuesta estructurada de la radio
    resp = ser.read(8)
    if len(resp) == 0:
        print("❌ El radio no respondió.")
    else:
        print(f"🎉 Respuesta del bootloader recibida en hexadecimal: {resp.hex()}")
        if len(resp) >= 3:
            if resp[1] == 0x52 and resp[2] == 0xE1:
                print("❌ El procesador sigue requiriendo la firma de modelo estricta.")
            else:
                print("✅ ¡El bootloader aceptó la firma!")
                
    ser.close()
except Exception as e:
    print(f"❌ Error: {e}")
