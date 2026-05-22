import serial
import time
import sys
import struct

def run_snooper(port='/dev/ttyUSB0', baudrate=115200):
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print("==================================================")
        print("📡 SNOOPER SPI BK4829 INICIADO")
        print(f"Escuchando tráfico en {port} a {baudrate} baudios...")
        print("==================================================")
        
        while True:
            # Sincronización o lectura de debug
            b = ser.read(1)
            if not b:
                continue
                
            if b[0] == 0xFE or b[0] == 0xFD:
                is_read = (b[0] == 0xFD)
                # Leer los siguientes 3 bytes: reg (1 byte), valHigh (1 byte), valLow (1 byte)
                data = ser.read(3)
                if len(data) == 3:
                    reg = data[0]
                    val = (data[1] << 8) | data[2]
                    
                    if is_read:
                        print(f"[{time.strftime('%H:%M:%S')}] BK4829_ReadReg(0x{reg:02X}) -> 0x{val:04X};")
                    else:
                        print(f"[{time.strftime('%H:%M:%S')}] BK4829_WriteReg(0x{reg:02X}, 0x{val:04X});")
            else:
                # Si no es un comando SPI, imprimirlo como texto (debug telemetry)
                try:
                    sys.stdout.write(b.decode('ascii'))
                    sys.stdout.flush()
                except UnicodeDecodeError:
                    pass
    except serial.SerialException as e:
        print(f"Error abriendo puerto serie: {e}")
    except KeyboardInterrupt:
        print("\nSnooper detenido por el usuario.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == '__main__':
    run_snooper()
