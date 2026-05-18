import serial
import time

port = '/dev/ttyUSB0'
baud = 38400

# XOR array from Jacek's k5prog
k5_xor_array = [
    0x16, 0x6c, 0x14, 0xe6, 0x2e, 0x91, 0x0d, 0x40,
    0x21, 0x35, 0xd5, 0x40, 0x13, 0x03, 0xe9, 0x80
]

def crc16_xmodem(data):
    crc = 0
    poly = 0x1021
    for byte in data:
        crc = crc ^ (byte << 8)
        for _ in range(8):
            crc = crc << 1
            if crc & 0x10000:
                crc = (crc ^ poly) & 0xFFFF
            else:
                crc = crc & 0xFFFF
    return crc

def obfuscate_cmd(cmd_payload):
    # clear cmd is payload + 2 bytes crc
    clear_crc = crc16_xmodem(cmd_payload)
    clear_payload_with_crc = bytearray(cmd_payload)
    clear_payload_with_crc.append(clear_crc & 0xFF)
    clear_payload_with_crc.append((clear_crc >> 8) & 0xFF)
    
    # obfuscate using XOR array
    obf_payload = bytearray()
    for i, b in enumerate(clear_payload_with_crc):
        obf_payload.append(b ^ k5_xor_array[i % len(k5_xor_array)])
        
    # package into frame
    frame = bytearray()
    frame.append(0xAB)
    frame.append(0xCD)
    frame.append(len(cmd_payload) & 0xFF)
    frame.append((len(cmd_payload) >> 8) & 0xFF)
    frame.extend(obf_payload)
    frame.append(0xDC)
    frame.append(0xBA)
    return frame

# Prepare Hello commands
uvk5_hello = [0x14, 0x05, 0x04, 0x00, 0x6a, 0x39, 0x57, 0x64]
uvk5_hello2 = [0x14, 0x05, 0x04, 0x00, 0x9f, 0x25, 0x5a, 0x64]
# Command 0x30 is used in flashing mode to present version
uvk5_flash_version = [0x30, 0x05, 0x10, 0x00, ord('*'), ord('.'), ord('0'), ord('1'), ord('.'), ord('2'), ord('3'), 0, 0, 0, 0, 0, 0, 0, 0, 0]

print("=== ENVIANDO PINGS AL BOOTLOADER PASIVO BAOFENG ===")

try:
    ser = serial.Serial(port, baud, timeout=2.0)
    ser.flushInput()
    ser.flushOutput()
    
    # Try Hello 1
    print("\n[1] Enviando Hello standard...")
    frame1 = obfuscate_cmd(uvk5_hello)
    print("Frame TX:", " ".join(f"{b:02x}" for b in frame1))
    ser.write(frame1)
    time.sleep(0.5)
    if ser.in_waiting > 0:
        res = ser.read(ser.in_waiting)
        print("✅ ¡RESPUESTA RECIBIDA!")
        print("Frame RX:", " ".join(f"{b:02x}" for b in res))
    else:
        print("❌ Sin respuesta a Hello standard.")
        
    # Try Hello 2
    print("\n[2] Enviando Hello2...")
    frame2 = obfuscate_cmd(uvk5_hello2)
    print("Frame TX:", " ".join(f"{b:02x}" for b in frame2))
    ser.write(frame2)
    time.sleep(0.5)
    if ser.in_waiting > 0:
        res = ser.read(ser.in_waiting)
        print("✅ ¡RESPUESTA RECIBIDA!")
        print("Frame RX:", " ".join(f"{b:02x}" for b in res))
    else:
        print("❌ Sin respuesta a Hello2.")

    # Try Flash Version Command 0x30
    print("\n[3] Enviando comando de versión de flash (0x30)...")
    frame3 = obfuscate_cmd(uvk5_flash_version)
    print("Frame TX:", " ".join(f"{b:02x}" for b in frame3))
    ser.write(frame3)
    time.sleep(0.5)
    if ser.in_waiting > 0:
        res = ser.read(ser.in_waiting)
        print("✅ ¡RESPUESTA RECIBIDA!")
        print("Frame RX:", " ".join(f"{b:02x}" for b in res))
    else:
        print("❌ Sin respuesta a comando de flash.")
        
    ser.close()
except Exception as e:
    print(f"Error: {e}")

print("\n=== PRUEBA DE PING FINALIZADA ===")
