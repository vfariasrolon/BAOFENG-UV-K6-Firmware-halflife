#!/bin/bash

# --- CONFIGURACIÓN DE COLORES Y ESTÉTICA ---
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # Sin color

clear
echo -e "${PURPLE}╔═══════════════════════════════════════════════════╗${NC}"
echo -e "${PURPLE}║${NC} ${CYAN}   🚀 BAOFENG UV-K6 CUSTOM FIRMWARE: HALF-LIFE   ${NC} ${PURPLE}║${NC}"
echo -e "${PURPLE}║${NC} ${CYAN}         Entorno de Compilación y Flasheo         ${NC} ${PURPLE}║${NC}"
echo -e "${PURPLE}╚═══════════════════════════════════════════════════╝${NC}"

# 1. Verificar si el entorno virtual venv existe
if [ ! -d "venv" ]; then
    echo -e "${YELLOW}[!] Entorno virtual de Python (venv) no encontrado.${NC}"
    echo -e "Ejecutando script de configuración inicial (setup.sh)..."
    chmod +x setup.sh
    ./setup.sh
    if [ $? -ne 0 ]; then
        echo -e "${RED}[✘] Error al configurar el entorno de desarrollo. Abortando.${NC}"
        exit 1
    fi
fi

# 2. Compilar el firmware con PlatformIO
echo -e "\n${BLUE}🔨 [1/3] Iniciando compilación de firmware...${NC}"
./venv/bin/pio run

if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✔] ¡Compilación exitosa!${NC}"
else
    echo -e "${RED}[✘] Error al compilar. Revisa los mensajes arriba.${NC}"
    exit 1
fi

# 3. Localizar el último binario compilado
LATEST_BIN=$(ls -t out/bfk6-*.bin 2>/dev/null | head -n 1)

if [ -z "$LATEST_BIN" ]; then
    echo -e "${RED}[✘] No se encontró ningún binario generado en la carpeta 'out/'.${NC}"
    exit 1
fi

echo -e "\n${BLUE}📦 [2/3] Binario más reciente localizado:${NC}"
echo -e "   ${GREEN}$LATEST_BIN (${NC}$(du -h "$LATEST_BIN" | cut -f1)${GREEN})${NC}"

# 4. Flashear la radio
echo -e "\n${BLUE}🔌 [3/3] Iniciando la herramienta de flasheo...${NC}"
echo -e "${YELLOW}Asegúrate de conectar tu cable de programación y encender la radio${NC}"
echo -e "${YELLOW}manteniendo presionado el botón PTT para entrar en Modo Flash.${NC}\n"

# Dar permisos inmediatos al puerto serial si existe
if [ -c /dev/ttyUSB0 ]; then
    echo -e "${CYAN}[i] Aplicando permisos de lectura/escritura a /dev/ttyUSB0...${NC}"
    sudo chmod 666 /dev/ttyUSB0 2>/dev/null
fi

# Ejecutar la herramienta de rescate interactiva
./venv/bin/python3 k6_rescue.py

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}================═══════════════════════════════════${NC}"
    echo -e "${GREEN}      ¡FIRMWARE INSTALADO Y COMPILADO CON ÉXITO!    ${NC}"
    echo -e "${GREEN}================═══════════════════════════════════${NC}"
    echo -e "${CYAN}¡Tu Baofeng UV-K6 está listo para la acción!${NC}\n"
else
    echo -e "\n${YELLOW}[!] Si el script falló por permisos del puerto, intenta ejecutar:${NC}"
    echo -e "    ${BLUE}sudo ./venv/bin/python3 k6_rescue.py${NC}\n"
fi
