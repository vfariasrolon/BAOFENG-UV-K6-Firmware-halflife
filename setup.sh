#!/bin/bash

# --- CONFIGURACIÓN DE COLORES PARA LA TERMINAL ---
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # Sin color

echo -e "${BLUE}===================================================${NC}"
echo -e "${GREEN}    Instalador de Entorno: Baofeng UV-K6 Half-Life  ${NC}"
echo -e "${BLUE}===================================================${NC}"

# 1. Comprobar sistema operativo (Debian/Ubuntu)
if [ -f /etc/debian_version ]; then
    echo -e "${GREEN}[✔] Sistema compatible detectado (Debian/Ubuntu).${NC}"
else
    echo -e "${YELLOW}[!] Advertencia: Este script fue diseñado para Ubuntu/Debian.${NC}"
    echo -e "Presiona [ENTER] para continuar de todos modos o Ctrl+C para cancelar..."
    read -r
fi

# 2. Instalar dependencias del sistema necesarias
echo -e "\n${BLUE}[1/4] Instalando dependencias del sistema...${NC}"
echo -e "${YELLOW}Se requieren permisos de administrador (sudo) para instalar dependencias de desarrollo y Python:${NC}"
sudo apt update
sudo apt install -y python3 python3-pip python3-venv build-essential git python3-setuptools

if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✔] Dependencias del sistema instaladas correctamente.${NC}"
else
    echo -e "${RED}[✘] Error al instalar dependencias del sistema.${NC}"
    exit 1
fi

# 3. Configurar entorno virtual de Python (venv)
echo -e "\n${BLUE}[2/4] Configurando el entorno virtual (venv)...${NC}"
if [ ! -d "venv" ]; then
    python3 -m venv venv
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[✔] Entorno virtual creado en ./venv${NC}"
    else
        echo -e "${RED}[✘] Error al crear el entorno virtual.${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}[-] El entorno virtual ya existe en ./venv, omitiendo creación.${NC}"
fi

# 4. Instalar dependencias de Python (PlatformIO y PySerial)
echo -e "\n${BLUE}[3/4] Instalando paquetes de Python (PlatformIO & PySerial)...${NC}"
./venv/bin/pip install --upgrade pip
./venv/bin/pip install -r requirements.txt

if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✔] PlatformIO y dependencias instaladas en el venv con éxito.${NC}"
else
    echo -e "${RED}[✘] Error al instalar dependencias de Python.${NC}"
    exit 1
fi

# 5. Resolver el problema de permisos de puerto serial (dialout)
echo -e "\n${BLUE}[4/4] Configurando permisos del puerto serial (USB)...${NC}"
echo -e "${YELLOW}En Ubuntu/Debian, para acceder a los programadores USB (/dev/ttyUSB0) sin ser root (sudo),${NC}"
echo -e "${YELLOW}tu usuario debe pertenecer al grupo 'dialout'. Agregando ahora:${NC}"

sudo usermod -a -G dialout "$USER"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}[✔] Tu usuario '$USER' fue agregado al grupo 'dialout'.${NC}"
else
    echo -e "${RED}[✘] Error al configurar el grupo dialout.${NC}"
fi

echo -e "\n${BLUE}===================================================${NC}"
echo -e "${GREEN}          ¡INSTALACIÓN COMPLETADA CON ÉXITO!        ${NC}"
echo -e "${BLUE}===================================================${NC}"
echo -e "${YELLOW}IMPORTANTE: Para que el cambio de permisos del USB${NC}"
echo -e "${YELLOW}surtan efecto, debes CERRAR SESIÓN en Ubuntu y volver${NC}"
echo -e "${YELLOW}a iniciar, o bien ejecutar el siguiente comando en${NC}"
echo -e "${YELLOW}tu terminal actual:${NC}"
echo -e "${BLUE}    newgrp dialout${NC}"
echo -e "\n${GREEN}Para compilar el proyecto ejecuta:${NC}"
echo -e "    ./venv/bin/pio run"
echo -e "\n${GREEN}Para flashear tu radio ejecuta:${NC}"
echo -e "    ./venv/bin/python3 k6_flash.py out/bfk6-*.bin"
echo -e "${BLUE}===================================================${NC}"
