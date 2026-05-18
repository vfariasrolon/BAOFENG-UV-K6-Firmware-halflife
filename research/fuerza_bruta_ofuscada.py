import serial
import time
import sys

def main():
    port = "/dev/ttyUSB0" if len(sys.argv) < 2 else sys.argv[1]
    dict_path = "diccionario_completo.txt"
    
    print("🧠 INYECTOR DE FIRMAS OFUSCADAS ANTI-GLITCH 🧠")
    
    # 1. Filtrar y cargar únicamente los candidatos que sigan el patrón ofuscado
    candidatos = []
    try:
        with open(dict_path, "r") as f:
            for line in f:
                word = line.strip()
                # Filtramos las cadenas típicas del ofuscador (que contienen qqr, ppq, mmn, etc.)
                if any(x in word for x in ["qqr", "ppq", "kkl", "mmn", "rrs"]) and len(word) <= 8:
                    if word.encode('ascii', errors='ignore') not in candidatos:
                        candidatos.append(word.encode('ascii'))
    except Exception as e:
        print(f"❌ No se pudo leer el diccionario: {e}")
        return

    print(f"📦 Se extrajeron {len(candidatos)} candidatos ofuscados listos para pruebas.")
    
    # 2. Abrir puerto serie PRIMERO para absorber transitorios DTR/RTS
    print(f"🔌 Inicializando descriptor de puerto en {port}...")
    try:
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = 115200
        ser.timeout = 0.2
        ser.dsrdtr = False
        ser.rtscts = False
        ser.open()
        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)
        ser.reset_input_buffer()
    except Exception as e:
        print(f"❌ Error al abrir el puerto serie: {e}")
        return

    print("\n🚨 PASO DE INICIALIZACIÓN:")
    print("  1. Asegúrate de que la radio esté APAGADA.")
    print("  2. Mantén presionados los DOS botones naranjas laterales.")
    print("  3. AHORA enciende la radio (Pantalla fija en UPDATE).")
    input("\n👉 Presiona ENTER aquí 2 segundos DESPUÉS de encender la radio para desatar la ráfaga...")

    print("\n🚀 Disparando ametralladora de handshakes...")
    
    for idx, cand in enumerate(candidatos):
        # Alinear estrictamente a 8 bytes con nulos como exige la estructura de la K6A
        modelo_padded = cand.ljust(8, b'\x00')[:8]
        
        # Primero el pulso unario
        ser.write(b"\x55")
        ser.flush()
        time.sleep(0.01)
        
        # Mandamos PROGRAM + Firma Ofuscada de 8 bytes + \x55 de control
        packet = b"PROGRAM" + modelo_padded + b"\x55"
        ser.write(packet)
        ser.flush()
        
        # Leemos la respuesta del procesador
        time.sleep(0.01)
        resp = ser.read(8)
        
        # Analizamos la cabecera
        if len(resp) >= 3:
            # Si el argumento deja de ser 0xE1, ¡ROMPISTE EL PROTOCOLO!
            if resp[2] != 0xE1:
                print(f"\n🎯 ¡MALDITA SEA, CORONADO!")
                print(f"Firma ganadora real de la matriz ofuscada: {cand.decode()}")
                print(f"Respuesta del procesador (Hex): {resp.hex()}")
                ser.close()
                return
        
        # Progreso rápido en la misma línea
        sys.stdout.write(f"\r🔍 Probando [{idx+1}/{len(candidatos)}]: {cand.decode().ljust(10)} -> Rechazado (0xE1)")
        sys.stdout.flush()

    print("\n\n❌ Ninguna de las cadenas ofuscadas quitó el candado del procesador.")
    ser.close()

if __name__ == "__main__":
    main()
