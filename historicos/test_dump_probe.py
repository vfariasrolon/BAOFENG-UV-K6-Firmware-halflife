import serial
import time
import sys

# Configuración del puerto serie (Ajusta /dev/ttyUSB0 según tu servidor)
PORT = '/dev/ttyUSB0'
BAUDRATE = 115200
TIMEOUT = 2

# Comandos estándar del protocolo del bootloader (basado en la arquitectura del chip)
CMD_HELLO = b'\x14\x05\x00\x00\x00\x00\x00\x00' # Comando de saludo estándar
CMD_READ_PROBE = b'\x1b\x05\x00\x00\x00\x00\x00\x00' # Intento de comando de lectura (Block 0)

def run_probe():
    print("=" * 50)
    print("📡 INICIANDO SCRIPT DE SONDEO DE MEMORIA (PROBE) 📡")
    print("=" * 50)
    print(f"[*] Conectando al puerto {PORT}...")
    
    try:
        # Abrimos el puerto serie
        ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
        time.sleep(0.1)
        
        # 1. PASO 1: Enviar el saludo para ver si el Bootloader responde
        print("[*] Enviando comando de saludo (HELLO)...")
        ser.write(CMD_HELLO)
        response_hello = ser.read(16) # Leemos la respuesta típica del firmware
        
        if not response_hello:
            print("[❌] ERROR: El radio no respondió al saludo. ¿Está en modo Bootloader (PTT + Encendido)?")
            ser.close()
            return
            
        print(f"[✅] ¡Conexión establecida! Respuesta del Bootloader (HEX): {response_hello.hex().upper()}")
        
        # 2. PASO 2: El disparo de prueba para el Volcado
        print("\n[*] ATENCIÓN: Intentando leer el primer bloque de la Flash (Dirección 0x0000)...")
        print("[*] Enviando petición de comando READ_PROBE...")
        ser.write(CMD_READ_PROBE)
        
        # Esperamos la respuesta del microcontrolador
        time.sleep(0.2)
        response_dump = ser.read(32) # Intentamos capturar los primeros bytes de datos
        
        print("-" * 50)
        if len(response_dump) > 0:
            print("[🎉] ¡Veredicto: COMPUERTAS ABIERTAS! El chip permitió la lectura.")
            print(f"[📊] Datos recuperados (Primeros bytes en HEX):\n{response_dump.hex().upper()}")
            print("-" * 50)
            print("[🚀] PASO SIGUIENTE: El hardware está liberado. Puedes correr el script completo para los 64 KB.")
        else:
            print("[❌] Veredicto: FLASH PROTEGIDA (Read-out Protection Activo).")
            print("[ℹ️] El chip ignoró o rechazó el comando de lectura por seguridad de fábrica.")
            print("-" * 50)
            print("[🔧] PASO SIGUIENTE: Confirmado el bloqueo. Mañana operamos por hardware con los pines SWD.")
            
        ser.close()
        
    except serial.SerialException as e:
        print(f"[❌] ERROR FÍSICO: No se pudo abrir el puerto serie. Verificas los permisos o el cable: {e}")
    except Exception as e:
        print(f"[❌] ERROR INESPERADO: {e}")

if __name__ == "__main__":
    run_probe()
