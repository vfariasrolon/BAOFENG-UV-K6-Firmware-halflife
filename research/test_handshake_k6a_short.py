import serial
import time
import sys

port = '/dev/ttyUSB0' if len(sys.argv) < 2 else sys.argv[1]

print("=== PRUEBA DE HANDSHAKE CORTO NATIVO PARA BF-K6A (14 BYTES) ===")
print("Asegúrate de que la radio esté en modo UPDATE.")
print(f"Puerto: {port}\n")

try:
    ser = serial.Serial(port, 115200, timeout=1.0)
    ser.dtr = True
    ser.rts = True
    ser.flushInput()
    ser.flushOutput()
    
    time.sleep(0.1)
    
    # --- PASO 1: Handshake Corto (7 bytes PROGRAM + 6 bytes BF-K6A + 1 byte 0x55) ---
    print("🤝 Probando Handshake Corto BF-K6A...")
    handshake = b"PROGRAM" + b"BF-K6A" + b"\x55"
    print(f"Bytes enviados (Hex): {handshake.hex()}")
    print(f"Longitud: {len(handshake)} bytes")
    
    ser.write(handshake)
    ser.flush()
    
    # Leemos la respuesta estructurada de la radio
    time.sleep(0.1)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"🎉 Respuesta del bootloader recibida en hexadecimal: {resp.hex()}")
        if 0x06 in resp:
            print("\n⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
            print("   ⭐⭐ SUCCESS! ¡EL BOOTLOADER ACEPTÓ EL HANDSHAKE! ⭐⭐")
            print("⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐⭐")
            
            # Send UPDATE
            print("📡 Enviando confirmación UPDATE...")
            ser.write(b"UPDATE")
            ser.flush()
            time.sleep(0.1)
            resp2 = ser.read(ser.in_waiting)
            print(f"🎉 Respuesta a UPDATE: {resp2.hex()}")
            if 0x06 in resp2:
                print("   ✅ ACK recibido para UPDATE. ¡Bootloader listo para flasheo activo!")
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
        print("❌ Sin respuesta de la radio.")
        
    ser.close()
except Exception as e:
    print(f"❌ Error: {e}")
