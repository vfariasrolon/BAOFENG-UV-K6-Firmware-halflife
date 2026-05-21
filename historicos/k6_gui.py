#!/usr/bin/env python3
import sys
import os
import time
import re
import queue
import threading
import serial
import serial.tools.list_ports
import customtkinter as ctk
from tkinter import filedialog, messagebox

# --- CONFIGURACIÓN DE APARIENCIA DE CUSTOMTKINTER ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# --- COLA THREAD-SAFE PARA LOGS DE LA INTERFAZ ---
log_queue = queue.Queue()

class QueueRedirector:
    """Redirige sys.stdout a una cola thread-safe que luego lee la interfaz GUI."""
    def __init__(self, q):
        self.queue = q

    def write(self, s):
        self.queue.put(s)

    def flush(self):
        pass

# --- CONSTANTES Y UTILERÍAS PROTOCOLO PROGRAMACIÓN (9600 BAUD) ---
K5_XOR_ARRAY = [
    0x16, 0x6c, 0x14, 0xe6, 0x2e, 0x91, 0x0d, 0x40,
    0x21, 0x35, 0xd5, 0x40, 0x13, 0x03, 0xe9, 0x80
]

def k5_xorarr(data: bytearray) -> bytearray:
    """(Des)ofusca datos usando XOR en base a la matriz del firmware original."""
    for i in range(len(data)):
        data[i] = data[i] ^ K5_XOR_ARRAY[i % len(K5_XOR_ARRAY)]
    return data

def k5_crc16xmodem(data: bytes) -> int:
    """Cálculo estándar del CRC-16 CCITT XMODEM."""
    crc = 0
    poly = 0x1021
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

def k5_obfuscate(payload: bytes) -> bytes:
    """Envuelve un payload claro en un datagrama ofuscado UV-K5/K6 listo para transmisión."""
    length = len(payload)
    obf_cmd = bytearray(length + 8)
    obf_cmd[0] = 0xab
    obf_cmd[1] = 0xcd
    obf_cmd[2] = length & 0xff
    obf_cmd[3] = (length >> 8) & 0xff
    obf_cmd[4:4+length] = payload
    
    c = k5_crc16xmodem(payload)
    obf_cmd[length + 4] = c & 0xff
    obf_cmd[length + 5] = (c >> 8) & 0xff
    
    # Ofuscar payload y CRC
    payload_and_crc = obf_cmd[4:length+6]
    k5_xorarr(payload_and_crc)
    obf_cmd[4:length+6] = payload_and_crc
    
    obf_cmd[length + 6] = 0xdc
    obf_cmd[length + 7] = 0xba
    return bytes(obf_cmd)

def k5_deobfuscate(data: bytes) -> bytes:
    """Verifica y descifra un datagrama recibido desde el radio."""
    if len(data) < 8:
        return None
    if data[0] != 0xab or data[1] != 0xcd:
        return None
    length = data[2] | (data[3] << 8)
    if len(data) < length + 8:
        return None
    if data[length + 6] != 0xdc or data[length + 7] != 0xba:
        return None
        
    payload_and_crc = bytearray(data[4:length+6])
    k5_xorarr(payload_and_crc)
    
    payload = payload_and_crc[:-2]
    # Retornamos el payload de la respuesta descifrada
    return bytes(payload)


# --- INTERFAZ GRÁFICA PRINCIPAL ---
class K6ControlPanelApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Baofeng UV-K6 Half-Life Control Panel")
        self.geometry("820x620")
        self.resizable(False, False)

        # Configurar grid de 2 columnas
        self.grid_columnconfigure(0, weight=4) # Panel de control
        self.grid_columnconfigure(1, weight=6) # Terminal / Consola de logs

        # --- PANEL IZQUIERDO (OPCIONES Y CONTROLES) ---
        self.left_frame = ctk.CTkFrame(self, corner_radius=15)
        self.left_frame.grid(row=0, column=0, padx=15, pady=15, sticky="nsew")
        
        # Título
        self.title_label = ctk.CTkLabel(self.left_frame, text="⚡ UV-K6 HALF-LIFE", font=ctk.CTkFont(size=20, weight="bold"))
        self.title_label.pack(padx=10, pady=15)

        # Selección de Puerto Serial
        self.port_label = ctk.CTkLabel(self.left_frame, text="🔌 Puerto Serial:", font=ctk.CTkFont(size=12, weight="bold"))
        self.port_label.pack(anchor="w", padx=20, pady=2)
        
        self.port_var = ctk.StringVar(value="Buscando...")
        self.port_menu = ctk.CTkOptionMenu(self.left_frame, variable=self.port_var, values=["Buscando..."])
        self.port_menu.pack(fill="x", padx=20, pady=5)
        
        self.refresh_btn = ctk.CTkButton(self.left_frame, text="🔄 Escanear Puertos USB", command=self.refresh_serial_ports, fg_color="transparent", border_width=1)
        self.refresh_btn.pack(fill="x", padx=20, pady=5)

        # Separador visual
        self.add_separator(self.left_frame)

        # Pruebas de Diagnóstico (Sondeo)
        self.diag_label = ctk.CTkLabel(self.left_frame, text="📡 Diagnóstico de Conexión:", font=ctk.CTkFont(size=12, weight="bold"))
        self.diag_label.pack(anchor="w", padx=20, pady=2)

        self.probe_btn = ctk.CTkButton(self.left_frame, text="🔍 Sondear Radio (A/B Test)", command=self.start_probe_thread, fg_color="#E67E22", hover_color="#D35400")
        self.probe_btn.pack(fill="x", padx=20, pady=10)

        # Separador visual
        self.add_separator(self.left_frame)

        # Selección de Firmware
        self.fw_label = ctk.CTkLabel(self.left_frame, text="📂 Archivo de Firmware (.bin):", font=ctk.CTkFont(size=12, weight="bold"))
        self.fw_label.pack(anchor="w", padx=20, pady=2)

        self.fw_path_entry = ctk.CTkEntry(self.left_frame, placeholder_text="Selecciona un archivo...")
        self.fw_path_entry.pack(fill="x", padx=20, pady=5)
        
        self.browse_btn = ctk.CTkButton(self.left_frame, text="📁 Buscar Firmware", command=self.browse_firmware_file, fg_color="#34495E", hover_color="#2C3E50")
        self.browse_btn.pack(fill="x", padx=20, pady=5)

        # Separador visual
        self.add_separator(self.left_frame)

        # Acciones de Grabado y Lectura
        self.action_label = ctk.CTkLabel(self.left_frame, text="⚡ Operaciones Especiales:", font=ctk.CTkFont(size=12, weight="bold"))
        self.action_label.pack(anchor="w", padx=20, pady=2)

        self.flash_btn = ctk.CTkButton(self.left_frame, text="⚡ Cargar Firmware (Flashear)", command=self.start_flash_thread, fg_color="#2ECC71", hover_color="#27AE60", font=ctk.CTkFont(weight="bold"))
        self.flash_btn.pack(fill="x", padx=20, pady=5)

        self.backup_btn = ctk.CTkButton(self.left_frame, text="💾 Extraer Respaldo EEPROM", command=self.start_backup_thread, fg_color="#9B59B6", hover_color="#8E44AD", font=ctk.CTkFont(weight="bold"))
        self.backup_btn.pack(fill="x", padx=20, pady=5)


        # --- PANEL DERECHO (CONSOLA DE LOGS Y PROGRESO) ---
        self.right_frame = ctk.CTkFrame(self, corner_radius=15)
        self.right_frame.grid(row=0, column=1, padx=15, pady=15, sticky="nsew")

        # Título Consola
        self.log_title = ctk.CTkLabel(self.right_frame, text="💻 Consola de Transmisión Real-time", font=ctk.CTkFont(size=14, weight="bold"))
        self.log_title.pack(padx=10, pady=10)

        # Text Box para logs (Estilo terminal hacker/matriz)
        self.log_text = ctk.CTkTextbox(self.right_frame, wrap="word", font=ctk.CTkFont(family="monospace", size=11), fg_color="#0D0D0D", text_color="#18FF6D")
        self.log_text.pack(fill="both", expand=True, padx=15, pady=10)
        self.log_text.configure(state="disabled")

        # Barra de progreso
        self.progress_label = ctk.CTkLabel(self.right_frame, text="Progreso General: 0%", font=ctk.CTkFont(size=12, weight="bold"))
        self.progress_label.pack(padx=15, pady=2, anchor="w")

        self.progress_bar = ctk.CTkProgressBar(self.right_frame, width=400)
        self.progress_bar.pack(fill="x", padx=15, pady=10)
        self.progress_bar.set(0.0)

        # Inicialización de puertos
        self.refresh_serial_ports()
        self.autodetect_latest_firmware()

        # Redirigir stdout a la cola
        sys.stdout = QueueRedirector(log_queue)

        # Lanzar el bucle de actualización periódica de la cola de logs
        self.poll_log_queue()

    def add_separator(self, parent):
        """Dibuja una línea sutil de separación."""
        sep = ctk.CTkFrame(parent, height=2, fg_color="#2A2A2A")
        sep.pack(fill="x", padx=25, pady=10)

    # --- ESCANEO DE PUERTOS SERIALES ---
    def refresh_serial_ports(self):
        """Escanea todos los puertos USB activos y los carga en el menú desplegable."""
        ports = serial.tools.list_ports.comports()
        port_list = [p.device for p in ports]
        
        if not port_list:
            self.port_var.set("No hay cables USB detectados")
            self.port_menu.configure(values=["No hay cables USB detectados"])
        else:
            self.port_var.set(port_list[0])
            self.port_menu.configure(values=port_list)
            self.append_log(f"🔍 Escaneo: Puertos USB disponibles detectados: {port_list}\n")

    def autodetect_latest_firmware(self):
        """Detecta automáticamente el binario más reciente en la carpeta 'out/'."""
        bin_dir = 'out/'
        if os.path.exists(bin_dir):
            bin_files = [f for f in os.listdir(bin_dir) if f.startswith('bfk6-') and f.endswith('.bin')]
            if bin_files:
                bin_files.sort(reverse=True)
                latest_fw = os.path.join(bin_dir, bin_files[0])
                self.fw_path_entry.delete(0, "end")
                self.fw_path_entry.insert(0, os.path.abspath(latest_fw))
                self.append_log(f"💡 Detección automática: Cargarás el firmware más reciente: {bin_files[0]}\n")

    def browse_firmware_file(self):
        """Abre un explorador de archivos nativo para seleccionar un firmware .bin."""
        file_path = filedialog.askopenfilename(
            title="Seleccionar firmware Baofeng UV-K6",
            filetypes=[("Firmware Files", "*.bin"), ("All Files", "*.*")]
        )
        if file_path:
            self.fw_path_entry.delete(0, "end")
            self.fw_path_entry.insert(0, file_path)
            self.append_log(f"📂 Archivo de firmware seleccionado: {os.path.basename(file_path)}\n")

    # --- ESCRITURA SEGURA EN LA PANTALLA DE LOGS ---
    def append_log(self, text):
        """Añade texto a la consola gráfica de terminal."""
        self.log_text.configure(state="normal")
        self.log_text.insert("end", text)
        self.log_text.see("end")
        self.log_text.configure(state="disabled")
        
        # Analizar el texto para actualizar barra de progreso gráfica
        match = re.search(r"(\d+)%", text)
        if match:
            percent = int(match.group(1))
            self.progress_bar.set(percent / 100.0)
            self.progress_label.configure(text=f"Progreso General: {percent}%")

    def poll_log_queue(self):
        """Monitorea la cola en busca de logs generados en hilos secundarios e imprime en pantalla."""
        while True:
            try:
                msg = log_queue.get_nowait()
                self.append_log(msg)
            except queue.Empty:
                break
        self.after(50, self.poll_log_queue)

    def disable_interface(self):
        """Bloquea los botones durante operaciones de hardware para evitar corrupción."""
        self.flash_btn.configure(state="disabled")
        self.backup_btn.configure(state="disabled")
        self.probe_btn.configure(state="disabled")
        self.refresh_btn.configure(state="disabled")
        self.browse_btn.configure(state="disabled")

    def enable_interface(self):
        """Habilita de nuevo la botonera al terminar."""
        self.flash_btn.configure(state="normal")
        self.backup_btn.configure(state="normal")
        self.probe_btn.configure(state="normal")
        self.refresh_btn.configure(state="normal")
        self.browse_btn.configure(state="normal")

    # --- HILO: ACCIÓN SONDEAR / PROBE ---
    def start_probe_thread(self):
        port = self.port_var.get()
        if "No hay" in port or "Buscando" in port:
            messagebox.showerror("Error de Hardware", "Por favor conecta y selecciona un puerto serial USB válido.")
            return
        
        self.disable_interface()
        self.progress_bar.set(0.0)
        self.progress_label.configure(text="Sondeando conexión...")
        self.append_log("\n" + "="*50 + "\n")
        self.append_log("📡 INICIANDO SONDEO DE DIAGNÓSTICO SERIAL\n")
        self.append_log("="*50 + "\n")

        thread = threading.Thread(target=self.run_probe_logic, args=(port,))
        thread.daemon = True
        thread.start()

    def run_probe_logic(self, port):
        # Códigos de saludo
        CMD_HELLO_A = b'\x14\x05\x00\x00\x00\x00\x00\x00' # Cortex estilo K6
        CMD_HELLO_B = b'\xaa\x55\x01\x00\x00\x00\x00\x56' # Baofeng Clásico

        def test_baud(baud):
            print(f"[*] Probando comunicación serial a {baud} Baudios...")
            try:
                ser = serial.Serial(port, baud, timeout=1.0)
                time.sleep(0.05)
                
                # Variación A
                print("    -> Enviando ráfaga de saludo Tipo A...")
                ser.write(CMD_HELLO_A)
                res = ser.read(16)
                if res:
                    ser.close()
                    return res, "Tipo A (Bootloader K5/K6)"
                
                # Variación B
                print("    -> Enviando ráfaga de saludo Tipo B...")
                ser.write(CMD_HELLO_B)
                res = ser.read(16)
                if res:
                    ser.close()
                    return res, "Tipo B (Baofeng Nativo)"
                
                ser.close()
            except Exception as e:
                print(f"    [❌] Error en el puerto a {baud}: {e}")
                if "Permission denied" in str(e) or "Errno 13" in str(e):
                    print(f"\n⚠️  ERROR DE PERMISOS DETECTADO EN {port}!")
                    print(f"👉 Ubuntu/Linux está bloqueando el acceso al puerto USB.")
                    print(f"👉 Para solucionarlo de inmediato, abre otra terminal y ejecuta:")
                    print(f"   sudo chmod 666 {port}")
                    print(f"👉 Para solucionarlo permanente, ejecuta 'newgrp dialout' o cierra sesión de Ubuntu.\n")
            return None, None

        # Probar primero 115200 (Modo Bootloader)
        res, tipo = test_baud(115200)
        
        # Si no, bajar a 9600 (Modo Programación estándar)
        if not res:
            res, tipo = test_baud(9600)

        print("-" * 50)
        if res:
            print(f"🎉 ¡ÉXITO! La radio respondió en el puerto {port}")
            print(f"📊 Protocolo detectado: {tipo}")
            print(f"📊 Respuesta HEX: {res.hex().upper()}")
            print("-" * 50)
            print("🚀 CONCLUSIÓN: El puerto de diagnóstico está vivo y conectado.")
        else:
            print("❌ SORDERA ABSOLUTA: El radio no respondió a ningún saludo.")
            print("💡 Recuerda tener la radio apagada, presionar PTT y encenderla")
            print("   si deseas que responda al modo bootloader (115200).")
            print("-" * 50)

        self.enable_interface()

    # --- HILO: ACCIÓN FLASHEAR ---
    def start_flash_thread(self):
        port = self.port_var.get()
        bin_path = self.fw_path_entry.get().strip()

        if "No hay" in port or "Buscando" in port:
            messagebox.showerror("Error de Hardware", "Por favor conecta y selecciona un puerto serial USB válido.")
            return
        if not bin_path or not os.path.exists(bin_path):
            messagebox.showerror("Archivo No Encontrado", "Por favor busca y selecciona un archivo firmware (.bin) válido.")
            return

        self.disable_interface()
        self.progress_bar.set(0.0)
        self.progress_label.configure(text="Progreso General: 0%")
        self.append_log("\n" + "="*50 + "\n")
        self.append_log("🚀 INICIANDO FLASHEO DE FIRMWARE EN SEGUNDO PLANO\n")
        self.append_log("="*50 + "\n")

        thread = threading.Thread(target=self.run_flash_logic, args=(port, bin_path))
        thread.daemon = True
        thread.start()

    def run_flash_logic(self, port, bin_path):
        try:
            # Importamos dinámicamente de k6_flash
            from k6_flash import flash_firmware
            success = flash_firmware(port, bin_path)
            
            if success:
                self.progress_bar.set(1.0)
                self.progress_label.configure(text="¡Flasheo Completado! 100%")
                messagebox.showinfo("Flasheo Exitoso", "¡El firmware de Half-Life se grabó en tu radio correctamente!\nLa radio debería reiniciarse.")
            else:
                self.progress_bar.set(0.0)
                self.progress_label.configure(text="Flasheo fallido")
                messagebox.showerror("Error", "Ocurrió un error al flashear la radio. Por favor lee los logs en la consola.")
        except Exception as e:
            print(f"\n❌ Error fatal durante la importación/ejecución del flasher: {e}")
            messagebox.showerror("Error", f"Ocurrió un error crítico: {e}")
        
        self.enable_interface()

    # --- HILO: ACCIÓN RESPALDO DE EEPROM (9600 BAUD) ---
    def start_backup_thread(self):
        port = self.port_var.get()
        if "No hay" in port or "Buscando" in port:
            messagebox.showerror("Error de Hardware", "Por favor conecta y selecciona un puerto serial USB válido.")
            return
        
        save_path = filedialog.asksaveasfilename(
            title="Guardar respaldo de EEPROM",
            defaultextension=".bin",
            filetypes=[("EEPROM Backup", "*.bin"), ("Raw Data", "*.raw")]
        )
        if not save_path:
            return

        self.disable_interface()
        self.progress_bar.set(0.0)
        self.progress_label.configure(text="Respaldando: 0%")
        self.append_log("\n" + "="*50 + "\n")
        self.append_log("💾 EXTRAER RESPALDO EEPROM COMPLETO (8KB)\n")
        self.append_log("="*50 + "\n")

        thread = threading.Thread(target=self.run_backup_logic, args=(port, save_path))
        thread.daemon = True
        thread.start()

    def run_backup_logic(self, port, save_path):
        try:
            print(f"🔌 Abriendo puerto serial {port} a 9600 baudios (Modo Programación)...")
            ser = serial.Serial(port, 9600, timeout=1.0)
            ser.dtr = True
            ser.rts = True
            ser.flush()
        except Exception as e:
            print(f"❌ Error al abrir el puerto serie: {e}")
            if "Permission denied" in str(e) or "Errno 13" in str(e):
                print(f"\n⚠️  ERROR DE PERMISOS DETECTADO EN {port}!")
                print(f"👉 Ubuntu/Linux está bloqueando el acceso al puerto USB.")
                print(f"👉 Para solucionarlo de inmediato, abre otra terminal y ejecuta:")
                print(f"   sudo chmod 666 {port}")
                print(f"👉 Para solucionarlo permanente, ejecuta 'newgrp dialout' o cierra sesión de Ubuntu.\n")
            self.enable_interface()
            return

        # Paso 1: Saludo en modo programación
        print("📡 Enviando protocolo de saludo en modo programación...")
        # uvk5_hello bytes claros
        hello_payload = bytes([0x14, 0x05, 0x04, 0x00, 0x6a, 0x39, 0x57, 0x64])
        obf_hello = k5_obfuscate(hello_payload)
        
        ser.write(obf_hello)
        ser.flush()
        
        # Leer respuesta (máximo 128 bytes)
        raw_res = ser.read(128)
        dec_res = k5_deobfuscate(raw_res)
        
        if not dec_res:
            print("❌ Error: La radio no respondió al saludo en modo programación.")
            print("💡 Asegúrate de tener la radio en MODO NORMAL (encendida normal, sin presionar PTT)")
            print("   y que el cable esté firmemente conectado.")
            ser.close()
            self.enable_interface()
            return
            
        print("✅ Saludo verificado. Modo programación activado.")
        
        # Paso 2: Bucle de lectura de EEPROM
        eeprom_data = bytearray()
        eeprom_total_size = 8192 # 8KB tamaño estándar
        block_size = 128         # Leemos en bloques de 128 bytes (máximo del buffer del radio)
        
        print(f"🚀 Leyendo {eeprom_total_size} bytes de memoria EEPROM...")
        
        for offset in range(0, eeprom_total_size, block_size):
            # uvk5_readmem1 estructurado
            readmem_payload = bytearray([
                0x1b,  0x05,  0x08,  0x00, 
                offset & 0xff,  (offset >> 8) & 0xff, 
                block_size,  0x00, 
                0x6a,  0x39,  0x57,  0x64
            ])
            obf_read = k5_obfuscate(bytes(readmem_payload))
            
            # Reintentos de lectura por ruido en el cable
            success_block = False
            for retry in range(3):
                ser.write(obf_read)
                ser.flush()
                
                # Formato de respuesta: Cabecera 4 bytes, payload (128 + 8 = 136 bytes), footer 2 bytes = 142 bytes aprox.
                res_block = ser.read(256)
                dec_block = k5_deobfuscate(res_block)
                
                if dec_block and len(dec_block) >= (block_size + 8) and dec_block[0] == 0x1c:
                    # Los datos claros leídos empiezan en el offset 8 del payload claro
                    data_read = dec_block[8:8+block_size]
                    eeprom_data.extend(data_read)
                    success_block = True
                    break
                else:
                    time.sleep(0.05) # Pequeña pausa antes del reintento
            
            if not success_block:
                print(f"\n❌ Error al leer el bloque en desplazamiento 0x{offset:04x}")
                ser.close()
                self.enable_interface()
                return
                
            percent = (offset + block_size) * 100 // eeprom_total_size
            sys.stdout.write(f"\r📥 Progreso de Respaldo: {percent}% | Dirección: 0x{offset:04x}")
            sys.stdout.flush()
            
        print("\n\n🎉 ¡Memoria EEPROM leída exitosamente!")
        
        # Enviar comando de reinicio a la radio para cerrar sesión limpia
        reset_payload = bytes([0xdd, 0x05, 0x00, 0x00])
        ser.write(k5_obfuscate(reset_payload))
        ser.flush()
        ser.close()

        # Guardar en archivo
        try:
            with open(save_path, 'wb') as f:
                f.write(eeprom_data)
            print(f"💾 Respaldo guardado correctamente en: {save_path}")
            messagebox.showinfo("Respaldo Completado", f"La memoria EEPROM (8KB) se respaldó exitosamente en:\n{save_path}")
        except Exception as e:
            print(f"❌ Error al escribir el archivo de respaldo: {e}")
            messagebox.showerror("Error de Escritura", f"No se pudo escribir el archivo: {e}")
            
        self.enable_interface()


if __name__ == "__main__":
    app = K6ControlPanelApp()
    app.mainloop()
