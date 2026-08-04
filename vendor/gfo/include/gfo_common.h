/* gfo_common.h - C89, no malloc. User-provided arena.
   Project: GFO (GameMaker-ish Object DSL)
*/
#ifndef GFO_COMMON_H
#define GFO_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* C89 friendly fixed types.
   C89 has no <stdint.h>, so we try to pick a real 32-bit type.
   You may override by defining:
     - GFO_U32_TYPE (e.g. unsigned long or unsigned int)
     - GFO_I32_TYPE (e.g. signed long or signed int)
*/
#include <limits.h>

typedef unsigned char  gfo_u8;
typedef signed char    gfo_i8;
typedef unsigned short gfo_u16;
typedef signed short   gfo_i16;

#ifndef GFO_U32_TYPE
# if defined(ULONG_MAX) && (ULONG_MAX == 0xFFFFFFFFUL)
#  define GFO_U32_TYPE unsigned long
# elif defined(UINT_MAX) && (UINT_MAX == 0xFFFFFFFFU)
#  define GFO_U32_TYPE unsigned int
# else
#  error "GFO: no 32-bit unsigned type found. Define GFO_U32_TYPE."
# endif
#endif

#ifndef GFO_I32_TYPE
# if defined(LONG_MAX) && (LONG_MAX == 0x7FFFFFFFL)
#  define GFO_I32_TYPE signed long
# elif defined(INT_MAX) && (INT_MAX == 0x7FFFFFFF)
#  define GFO_I32_TYPE signed int
# else
#  error "GFO: no 32-bit signed type found. Define GFO_I32_TYPE."
# endif
#endif

typedef GFO_U32_TYPE gfo_u32;
typedef GFO_I32_TYPE gfo_i32;

#ifndef GFO_NULL
#define GFO_NULL ((void*)0)
#endif

#ifndef GFO_TRUE
#define GFO_TRUE 1
#define GFO_FALSE 0
#endif

typedef struct { const char* ptr; gfo_u16 len; } gfo_str;

/* Compare slices (case-sensitive) */
int gfo_str_eq(gfo_str a, gfo_str b);
gfo_u32 gfo_hash_fnv1a(gfo_str s);

/* Arena (no malloc). Caller provides memory block. */
typedef struct {
  gfo_u8*  base;
  gfo_u32  size;
  gfo_u32  used;
} gfo_arena;

void* gfo_arena_alloc(gfo_arena* a, gfo_u32 bytes, gfo_u32 align);

/* Errors */
enum {
  GFO_OK = 0,
  GFO_ERR_OOM = -1,
  GFO_ERR_LEX = -2,
  GFO_ERR_PARSE = -3,
  GFO_ERR_COMPILE = -4,
  GFO_ERR_RANGE = -5
};

/* Helpers */
#define GFO_MIN(a,b) ((a)<(b)?(a):(b))
#define GFO_MAX(a,b) ((a)>(b)?(a):(b))

#ifdef __cplusplus
}
#endif
#endif /* GFO_COMMON_H */
