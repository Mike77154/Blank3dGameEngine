#ifndef BMP_TYPES_H
#define BMP_TYPES_H

#include <limits.h>
#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#if UCHAR_MAX != 255
#error BMP89 requires 8-bit unsigned char
#endif
#if USHRT_MAX != 65535U
#error BMP89 requires 16-bit unsigned short
#endif
#if UINT_MAX != 4294967295U
#error BMP89 requires 32-bit unsigned int
#endif
#if INT_MAX != 2147483647
#error BMP89 requires 32-bit signed int
#endif

typedef unsigned char  bmp_u8;
typedef unsigned short bmp_u16;
typedef unsigned int   bmp_u32;
typedef signed int     bmp_s32;

/* Optional 16.16 fixed-point scalar for integrations that need fractions. */
typedef bmp_s32 bmp_fx16_16;

#ifdef __cplusplus
}
#endif

#endif
