#ifndef _PUBLTYPE_H
#define _PUBLTYPE_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef bool     Boolean;
typedef char     String;

#define TRUE  true
#define FALSE false
// Renombramos ERROR a ERR_VAL para evitar conflictos con el driver del chip que usa "ERROR" como enum
#define ERR_VAL 1 

#endif
