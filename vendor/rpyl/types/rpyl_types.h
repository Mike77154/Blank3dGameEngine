#ifndef RPYL_TYPES_H
#define RPYL_TYPES_H

/*
    Exact-width integer aliases without requiring C99 <stdint.h>.

    RPYL serializes bytecode as 32-bit words and uses signed 16.16 fixed point,
    so silently mapping these names to a 64-bit long changes both behaviour and
    structure sizes on LP64 hosts.  Select an actually 32-bit C89 type instead.
*/
#include <limits.h>

#if UCHAR_MAX == 0xFFU
typedef unsigned char rpyl_u8;
typedef signed char rpyl_i8;
#else
#error RPYL_requires_8_bit_char
#endif

#if USHRT_MAX == 0xFFFFU
typedef unsigned short rpyl_u16;
typedef signed short rpyl_i16;
#elif UINT_MAX == 0xFFFFU
typedef unsigned int rpyl_u16;
typedef signed int rpyl_i16;
#else
#error RPYL_requires_a_16_bit_integer_type
#endif

#if UINT_MAX == 0xFFFFFFFFUL
typedef unsigned int rpyl_u32;
typedef signed int rpyl_i32;
#elif ULONG_MAX == 0xFFFFFFFFUL
typedef unsigned long rpyl_u32;
typedef signed long rpyl_i32;
#else
#error RPYL_requires_a_32_bit_integer_type
#endif

#define RPYL_U32_MAX ((rpyl_u32)0xFFFFFFFFUL)
#define RPYL_I32_MAX ((rpyl_i32)2147483647L)
#define RPYL_I32_MIN ((rpyl_i32)(-2147483647L - 1L))

typedef rpyl_i32 rpyl_fixed;

#endif
