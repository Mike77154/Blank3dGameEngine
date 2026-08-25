#ifndef WOFF1_TYPES_H
#define WOFF1_TYPES_H

#include <stddef.h>
#include <limits.h>

#if UINT_MAX >= 4294967295u
typedef unsigned int woff1_u32;
typedef int woff1_s32;
#else
typedef unsigned long woff1_u32;
typedef long woff1_s32;
#endif

typedef unsigned short woff1_u16;
typedef unsigned char woff1_u8;

#define WOFF1_U32_MASK 0xfffffffful

#endif
