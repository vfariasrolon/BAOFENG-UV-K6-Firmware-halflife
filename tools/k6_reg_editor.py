#!/usr/bin/env python3
import sys
import serial
import time
import os

# --- ESTÉTICA Y COLORES ---
GREEN = '\033[0;32m'
BLUE = '\033[0;34m'
YELLOW = '\033[1;33m'
RED = '\033[0;31m'
CYAN = '\033[0;36m'
NC = '\033[0m'

def print_header():
    os.system('clear' if os.name == 'posix' else 'cls')
    print(f"{BLUE}==================================================={NC}")
    print(f"{GREEN}    HALF-LIFE: DEBUGER INTERACTIVO DE REGISTROS    {NC}")
    print(f"{GREEN}          Transceptor Beken BK4819 / BK4829        {NC}")
    print(f"{BLUE}==================================================={NC}")
    print(f"{YELLOW}Modo de uso:{NC}")
    print(f"  {CYAN}r <reg>{NC}          -> Leer registro (Ejemplo: {CYAN}r 0x30{NC} o {CYAN}r 48{NC})")
    print(f"  {CYAN}w <reg> <val>{NC}    -> Escribir registro (Ejemplo: {CYAN}w 0x30 0x0007{NC})")
    print(f"  {CYAN}scan{NC}             -> Volcar/Escanear todos los registros (0x00 a 0x7F)")
    print(f"  {CYAN}help{NC}             -> Mostrar esta ayuda")
    print(f"  {CYAN}exit{NC}             -> Salir de la herramienta")
    print(f"{BLUE}==================================================={NC}\n")

def parse_int(val_str):
    """Soporta valores tanto decimales como hexadecimales (0x...)"""
    val_str = val_str.strip()
    if val_str.lower().startswith('0x'):
        return int(val_str, 16)
    return int(val_str)

def read_register(ser, reg):
    # Enviar comando de lectura: [0xFD, REG]
    packet = bytes([0xFD, reg])
    ser.write(packet)
    ser.flush()
    
    # Leer respuesta (esperamos 4 bytes: [0xFD, REG, VAL_MSB, VAL_LSB])
    response = ser.read(4)
    if len(response) == 4 and response[0] == 0xFD and response[1] == reg:
        val = (response[2] << 8) | response[3]
        return val
    return None

def write_register(ser, reg, val):
    val_msb = (val >> 8) & 0xFF
    val_lsb = val & 0xFF
    # Enviar comando de escritura: [0xFE, REG, VAL_MSB, VAL_LSB]
    packet = bytes([0xFE, reg, val_msb, val_lsb])
    ser.write(packet)
    ser.flush()
    
    # Leer respuesta (esperamos ACK de 4 bytes: [0xFE, REG, VAL_MSB, VAL_LSB])
    response = ser.read(4)
    if len(response) == 4 and response[0] == 0xFE and response[1] == reg:
        ack_val = (response[2] << 8) | response[3]
        return ack_val == val
    return False

def main():
    port = "/dev/ttyUSB0"
    if len(sys.argv) > 1:
        port = sys.argv[1]
        
    print(f"🔌 Conectando al puerto {port} a 115200 baudios...")
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
    except Exception as e:
        print(f"{RED}❌ Error al abrir el puerto serial {port}: {e}{NC}")
        print(f"{YELLOW}Sugerencia: Asegúrate de que el cable está bien conectado y ejecuta:{NC}")
        print(f"   sudo chmod 666 {port}")
        return

    print_header()

    while True:
        try:
            cmd_line = input(f"{GREEN}halflife-dbg>{NC} ").strip()
            if not cmd_line:
                continue
                
            parts = cmd_line.split()
            cmd = parts[0].lower()
            
            if cmd == 'exit' or cmd == 'q':
                print(f"\n👋 ¡Hasta luego Víctor! Cerrando puerto y saliendo.")
                break
                
            elif cmd == 'help' or cmd == '?':
                print_header()
                
            elif cmd == 'r':
                if len(parts) < 2:
                    print(f"{RED}❌ Uso incorrecto. Ejemplo: r 0x30{NC}")
                    continue
                try:
                    reg = parse_int(parts[1])
                    if reg < 0 or reg > 0xFF:
                        print(f"{RED}❌ Dirección de registro inválida (debe ser 0-255).{NC}")
                        continue
                    
                    val = read_register(ser, reg)
                    if val is not None:
                        print(f"📖 {GREEN}Registro 0x{reg:02X} ({reg}) = 0x{val:04X} ({val}){NC}")
                    else:
                        print(f"{RED}⚠️ Sin respuesta del radio (¿está encendido y conectado?).{NC}")
                except ValueError:
                    print(f"{RED}❌ Valor numérico inválido.{NC}")
                    
            elif cmd == 'w':
                if len(parts) < 3:
                    print(f"{RED}❌ Uso incorrecto. Ejemplo: w 0x30 0x0007{NC}")
                    continue
                try:
                    reg = parse_int(parts[1])
                    val = parse_int(parts[2])
                    if reg < 0 or reg > 0xFF:
                        print(f"{RED}❌ Dirección de registro inválida.{NC}")
                        continue
                    if val < 0 or val > 0xFFFF:
                        print(f"{RED}❌ Valor de registro inválido (16-bits máx: 0xFFFF).{NC}")
                        continue
                        
                    success = write_register(ser, reg, val)
                    if success:
                        print(f"✍️ {GREEN}¡Escritura exitosa! Reg 0x{reg:02X} = 0x{val:04X}{NC}")
                    else:
                        print(f"{RED}❌ Error o sin confirmación de escritura del radio.{NC}")
                except ValueError:
                    print(f"{RED}❌ Valor numérico inválido.{NC}")
                    
            elif cmd == 'scan':
                print(f"\n🔍 {BLUE}Iniciando volcado completo de registros (0x00 - 0x7F)...{NC}")
                readable_count = 0
                for r in range(0x80):
                    val = read_register(ser, r)
                    if val is not None:
                        readable_count += 1
                        print(f"  Reg 0x{r:02X} ({r:3d}) = 0x{val:04X} ({val:5d})")
                        time.sleep(0.002)
                if readable_count == 0:
                    print(f"{RED}❌ No se pudo leer ningún registro. Verifica la conexión.{NC}")
                else:
                    print(f"✅ Escaneo completado. {GREEN}{readable_count} registros leídos.{NC}\n")
            else:
                print(f"{RED}❌ Comando desconocido: '{cmd}'. Escribe 'help' para ver los comandos.{NC}")
                
        except KeyboardInterrupt:
            print(f"\n👋 Saliendo de la herramienta...")
            break
        except Exception as e:
            print(f"{RED}💥 Error: {e}{NC}")

    ser.close()

if __name__ == '__main__':
    main()
