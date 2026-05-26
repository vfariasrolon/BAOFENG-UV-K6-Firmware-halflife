import sys
try:
    from PIL import Image
except ImportError:
    print("Error: Necesitas instalar Pillow. Ejecuta: pip install Pillow")
    sys.exit(1)

def convert_image_to_c_array(image_path, output_path="logo_output.c"):
    try:
        # 1. Abrir imagen y forzar a 128x64 monocromática
        img = Image.open(image_path).convert('1').resize((128, 64))
        pixels = img.load()
        c_array = []

        # 2. Escaneo Horizontal Puro, Fila por Fila (y de 0 a 63)
        for y in range(64):
            # 16 bytes por fila (x_byte de 0 a 15)
            for x_byte in range(16):
                byte = 0
                # 8 bits por byte (bit de 0 a 7, izquierda a derecha)
                for bit in range(8):
                    px = pixels[x_byte * 8 + bit, y]
                    # Si el pixel es blanco/encendido, seteamos el bit
                    # MSB First: el primer pixel va en el bit 0x80 (128)
                    if px:
                        byte |= (0x80 >> bit)
                c_array.append(byte)

        # 3. Formatear y guardar como archivo C
        with open(output_path, 'w') as f:
            f.write('#include "logo.h"\n#include "Sc5260.h"\n\nstatic const unsigned char halflife_logo_128x64[1024] = {\n')
            for i in range(0, 1024, 16):
                row = c_array[i:i+16]
                s = ', '.join(f'0x{b:02X}' for b in row)
                f.write(f'    {s},\n')
            f.write("};\n\nvoid LCD_DrawLogo(void)\n{\n    U8 x, y, byte, bit;\n    \n    for (y = 0; y < 64; y++) {\n        for (x = 0; x < 128; x++) {\n            byte = halflife_logo_128x64[(y * 16) + (x / 8)];\n            bit = (byte & (0x80 >> (x % 8))) ? 1 : 0;\n            LCD_DrawPixel(x, y, bit);\n        }\n    }\n}\n")
        
        print(f"¡Éxito! Array de 1024 bytes guardado en {output_path}")
        print("Copia ese array y pégalo directamente en src/Driver/logo.c")

    except Exception as e:
        print(f"Error procesando la imagen: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python img2c.py <tu_imagen.png>")
    else:
        convert_image_to_c_array(sys.argv[1])
