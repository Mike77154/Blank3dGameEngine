#ifndef VP_TYPES_H
#define VP_TYPES_H

#include <limits.h>

/* ---- fixed-width-ish typedefs without <stdint.h> (C89-friendly) ---- */

typedef signed char     vp_i8;
typedef unsigned char   vp_u8;
typedef signed short    vp_i16;
typedef unsigned short  vp_u16;

/* 32-bit (choose int or long depending on platform) */
#if (UINT_MAX == 4294967295UL)
typedef signed int      vp_i32;
typedef unsigned int    vp_u32;
#elif (ULONG_MAX == 4294967295UL)
typedef signed long     vp_i32;
typedef unsigned long   vp_u32;
#else
# error "vp requires a 32-bit 'unsigned int' or 'unsigned long'"
#endif

#if (UCHAR_MAX != 255)
# error "vp requires 8-bit unsigned char"
#endif
#if (USHRT_MAX < 65535)
# error "vp requires unsigned short >= 16 bits"
#endif

#define VP_UNUSED(x) (void)(x)

/* One C89-safe forward typedef shared by all public headers. */
#ifndef VP_WORLD_FORWARD_DECLARED
#define VP_WORLD_FORWARD_DECLARED
struct vpWorld;
typedef struct vpWorld vpWorld;
#endif

#endif /* VP_TYPES_H */
