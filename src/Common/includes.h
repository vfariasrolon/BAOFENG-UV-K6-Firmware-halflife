#ifndef _INCLUDES_H
#define _INCLUDES_H

/*
 * includes.h — Fase 1: Heartbeat Minimalista
 *
 * Versión de bootstrapping: reemplaza los headers de hardware originales
 * (kd32f3xx.h, Board.h, PublType.h, keyboard.h, Beep.h, etc.) por
 * stub_constants.h, que provee tipos, constantes y structs mínimas.
 *
 * Para restaurar la build completa, revertir a los includes originales
 * una vez que los módulos de Driver/ y Common/ sean recuperados.
 */

/* --- Stdlib — siempre disponibles --- */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --- Fase 1: Constantes, tipos y globals stub --- */
#include "stub_constants.h"
#include "../Driver/keyboard.h"

/* --- Fase 1: Prototipos de todas las funciones externas --- */
#include "prototypes.h"

/* --- AppHalfLife: lógica de misión Half-Life --- */
#include "AppHalfLife.h"

/* --- GUI mínima --- */
#include "DisplayMenu.h"

#endif /* _INCLUDES_H */
