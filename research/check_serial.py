import serial
import time
import sys

port = '/dev/ttyUSB0'
baudrates = [38400, 115200]

print("=== DIAGNÓSTICO DE CONEXIÓN SERIE DE RADIO BAOFENG ===")

for baud in baudrates:
    print(f"\nProbando comunicación a {baud} baudios...")
    try:
        ser = serial.Serial(port, baud, timeout=1.0)
        # Limpiar buffers
        ser.flushInput()
        ser.flushOutput()
        
        # Escuchar por 3 segundos
        start_time = time.time()
        buffer = bytearray()
        print("Escuchando ráfagas del bootloader (deberían llegar paquetes 0x18 periódicamente)...")
        
        while time.time() - start_time < 3.0:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
        ser.close()
        
        if len(buffer) > 0:
            print(f"✅ ¡Se recibieron {len(buffer)} bytes a {baud} baudios!")
            print("Datos en hexadecimal:")
            print(" ".join(f"{b:02x}" for b in buffer[:64]))
            if len(buffer) > 64:
                print("... (truncado)")
        else:
            print(f"❌ Sin respuesta/silencio a {baud} baudios.")
            
    except Exception as e:
        print(f"⚠️ Error al abrir el puerto a {baud} baudios: {e}")

print("\n=== DIAGNÓSTICO FINALIZADO ===")
