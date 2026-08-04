/* sff_types.h - C89 integer types, integer-only/fixed-point friendly */
#ifndef SFF_TYPES_H
#define SFF_TYPES_H

#include <limits.h>

#if CHAR_BIT != 8
# error "sff: requires 8-bit bytes."
#endif

typedef unsigned char  sff_u8;
typedef signed char    sff_s8;

#if USHRT_MAX == 0xFFFFu
typedef unsigned short sff_u16;
typedef signed short   sff_s16;
#else
# error "sff: requires 16-bit unsigned short."
#endif

#if UINT_MAX == 0xFFFFFFFFu
typedef unsigned int   sff_u32;
typedef signed int     sff_s32;
#elif ULONG_MAX == 0xFFFFFFFFul
typedef unsigned long  sff_u32;
typedef signed long    sff_s32;
#else
# error "sff: requires 32-bit unsigned int or unsigned long."
#endif

typedef int sff_bool;
#define SFF_FALSE 0
#define SFF_TRUE  1

#endif /* SFF_TYPES_H */
