import serial
import time
import sys

def calc_crc(data):
    crc = 0x0000
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc

def probar_handshake(ser, payload_modelo):
    # Paso 1: Pulso unario
    ser.write(b"\x55")
    ser.flush()
    time.sleep(0.05)
    
    # Paso 2: Construir la ráfaga exacta (PROGRAM + MODELO + ACTIVACIÓN)
    # Ajustamos estrictamente a la estructura que pide el procesador
    payload = b"PROGRAM" + payload_modelo
    ser.write(payload)
    ser.flush()
    
    # Leer respuesta (8 bytes de la trama estructurada)
    resp = ser.read(8)
    return resp

def main():
    port = "/dev/ttyUSB0"
    print("📡 ESCÁNER DE VARIANTES DE FIRMA V02 NATIVA 📡")
    print("Asegúrate de tener la radio en modo UPDATE en cada intento.\n")
    
    # Generamos los candidatos basados en la cadena real número 217
    candidatos = [
        b"BFK6_V02",               # 8 bytes exactos
        b"K6_V02\x00\x00",         # 8 bytes con nulos
        b"BFK6_B02",               # Variante de Boot V02
        b"BFK6Boot",               # Primeros 8 bytes de BFK6_Bootloader
        b"BootV02\x00",            # Variante corta
        b"BFK6_BootloaderV02\x55", # Cadena cruda completa + activación
        b"BFK6_BootloaderV02"      # Cadena cruda completa
    ]
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
    except Exception as e:
        print(f"❌ No se pudo abrir el puerto: {e}")
        return

    for idx, cand in enumerate(candidatos):
        print(f"\n[Variante {idx+1}/{len(candidatos)}]: {cand}")
        print("👉 Apaga y enciende la radio en modo UPDATE (botones naranjas).")
        input("   Presiona ENTER cuando la radio esté lista...")
        
        res = probar_handshake(ser, cand)
        
        if len(res) > 0:
            print(f"🎉 Respuesta en bruto (Hex): {res.hex()}")
            # Si la respuesta NO es el error de firma e1 (aa52e1...), ¡lo logramos!
            if not (res[1] == 0x52 and res[2] == 0xE1):
                print("🎯 ¡BINGO! El código de error desapareció. Esta es la firma correcta.")
                print(f"Guarda esta cadena para el flasher final: {cand}")
                ser.close()
                return
            else:
                print("❌ Firma rechazada (Error E1).")
        else:
            print("❌ Sin respuesta (Timeout).")
            
    ser.close()
    print("\n❌ Se terminaron las variantes del diccionario.")

if __name__ == "__main__":
    main()
