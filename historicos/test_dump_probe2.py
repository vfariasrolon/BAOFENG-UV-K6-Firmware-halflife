import serial
import time
import sys

PORT = '/dev/ttyUSB0'  # Ajusta si tu Linux cambió el puerto a ttyUSB1
TIMEOUT = 2

# Intentaremos con los dos comandos de saludo más comunes en arquitecturas Baofeng/Cortex
CMD_HELLO_A = b'\x14\x05\x00\x00\x00\x00\x00\x00' # Estilo Quansheng/K6
CMD_HELLO_B = b'\xaa\x55\x01\x00\x00\x00\x00\x56' # Estilo Baofeng clásico/BaoFw

def try_handshake(baudrate):
    print(f"\n[*] Probando conexión a {baudrate} Baudios...")
    try:
        ser = serial.Serial(PORT, baudrate, timeout=TIMEOUT)
        time.sleep(0.1)
        
        # Intentando Variante de Saludo A
        print("    -> Enviando ráfaga de saludo tipo A...")
        ser.write(CMD_HELLO_A)
        res = ser.read(16)
        if res:
            return ser, res, "Tipo A (K6/Cortex)"
            
        # Intentando Variante de Saludo B
        print("    -> Enviando ráfaga de saludo tipo B...")
        ser.write(CMD_HELLO_B)
        res = ser.read(16)
        if res:
            return ser, res, "Tipo B (Baofeng Nativo)"
            
        ser.close()
    except Exception as e:
        print(f"    [❌] Error en el puerto a {baudrate}: {e}")
    return None, None, None

def run_probe():
    print("=" * 50)
    print("📡 SONDEO DE INTERFAZ SERIE: BAOFENG UV-5HR 📡")
    print("=" * 50)
    print(f"[*] Escaneando hardware en {PORT}...")
    
    # Probamos primero la velocidad rápida de los firmwares más nuevos
    ser, response, estilo = try_handshake(115200)
    
    # Si no responde, bajamos la velocidad a la clásica de programación de canales
    if not response:
        ser, response, estilo = try_handshake(9600)
        
    print("-" * 50)
    if response:
        print(f"[🎉] ¡EL UV-5HR RESPONDIÓ AL SALUDO!")
        print(f"[📊] Protocolo detectado: {estilo}")
        print(f"[📊] Respuesta en HEX: {response.hex().upper()}")
        print("-" * 50)
        print("[🚀] CONCLUSIÓN: El puerto de diagnóstico está vivo. Podemos intentar mapear comandos de lectura.")
        ser.close()
    else:
        print("[❌] SORDERA ABSOLUTA: El radio no respondió a ninguna combinación.")
        print("[ℹ️] Esto puede significar que el UV-5HR requiere una combinación de botones diferente")
        print("     para entrar al Bootloader, o el protocolo serie usa una llave de encriptación distinta.")
        print("-" * 50)

if __name__ == "__main__":
    run_probe()
