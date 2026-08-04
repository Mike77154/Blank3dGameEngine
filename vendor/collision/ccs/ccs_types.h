#ifndef CCS_TYPES_H
#define CCS_TYPES_H

/*
    ccs_types.h
    -----------
    Tipos básicos para toolchains C89 (incluyendo PS1).

    Objetivo:
    - Evitar <stdint.h> (no es C89)
    - Mantener tamaños predecibles para fixed-point y flags.

    Suposiciones por defecto (válidas para PS1 y la mayoría de targets modernos):
    - sizeof(char)  == 1
    - sizeof(short) == 2
    - sizeof(int)   == 4

    Si tu plataforma NO cumple esto, define CCS_I32 / CCS_U32 antes de incluir.
*/

/* ----------------------------
   32-bit signed/unsigned
   ---------------------------- */

#ifndef CCS_I32
typedef int ccs_i32;
#else
typedef CCS_I32 ccs_i32;
#endif

#ifndef CCS_U32
typedef unsigned int ccs_u32;
#else
typedef CCS_U32 ccs_u32;
#endif

/* ----------------------------
   Compile-time checks (C89)
   ---------------------------- */

typedef char ccs__i32_must_be_32bit[(sizeof(ccs_i32) == 4) ? 1 : -1];
typedef char ccs__u32_must_be_32bit[(sizeof(ccs_u32) == 4) ? 1 : -1];

/* ----------------------------
   Integer limits (avoid <limits.h>)
   ---------------------------- */

#define CCS_I32_MAX  2147483647
#define CCS_I32_MIN  (-2147483647 - 1)

#endif /* CCS_TYPES_H */
