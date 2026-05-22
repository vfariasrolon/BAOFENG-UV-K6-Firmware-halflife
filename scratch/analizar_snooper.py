import re
import sys

def parse_log(filepath):
    lines = open(filepath).read().splitlines()
    for line in lines:
        if "BK4829_WriteReg(0x" in line:
            print(line)

parse_log("salida.txt")
