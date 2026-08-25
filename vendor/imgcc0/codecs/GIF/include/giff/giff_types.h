#ifndef GIFF_TYPES_H
#define GIFF_TYPES_H

typedef unsigned char  giff_u8;
typedef signed char    giff_s8;
typedef unsigned short giff_u16;
typedef signed short   giff_s16;
typedef unsigned int   giff_u32;
typedef signed int     giff_s32;

typedef int giff_bool;

#define GIFF_FALSE 0
#define GIFF_TRUE  1

typedef struct giff_rgb8 {
    giff_u8 r;
    giff_u8 g;
    giff_u8 b;
} giff_rgb8;

typedef struct giff_rgba8 {
    giff_u8 r;
    giff_u8 g;
    giff_u8 b;
    giff_u8 a;
} giff_rgba8;

#endif
