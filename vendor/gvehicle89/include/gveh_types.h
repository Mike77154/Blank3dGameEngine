#ifndef GVEH_TYPES_H
#define GVEH_TYPES_H

/* gvehicle89 - C89, fixed point, no dynamic storage. */

typedef signed char        gveh_i8;
typedef unsigned char      gveh_u8;
typedef short              gveh_i16;
typedef unsigned short     gveh_u16;
/* Selectable 32-bit signed/unsigned storage. Defaults live in gveh_config.h. */
#include "gveh_config.h"
typedef GVEH_I32_TYPE      gveh_i32;
typedef GVEH_U32_TYPE      gveh_u32;

/* Compile-time ABI guards: gveh_fx must stay 32-bit for deterministic saves/replays. */
typedef char gveh_static_assert_i32_is_4[(sizeof(gveh_i32) == 4) ? 1 : -1];
typedef char gveh_static_assert_u32_is_4[(sizeof(gveh_u32) == 4) ? 1 : -1];

typedef gveh_i32           gveh_fx;

#define GVEH_OK             0
#define GVEH_ERR           -1

#define GVEH_FX_SHIFT       8
#define GVEH_FX_ONE         ((gveh_fx)1 << GVEH_FX_SHIFT)
#define GVEH_FX_HALF        (GVEH_FX_ONE >> 1)
#define GVEH_FX_EPS         1

#define GVEH_MAX_WHEELS     8
#define GVEH_MAX_MASSPTS    16
#define GVEH_MAX_BUOYS      8
#define GVEH_MAX_SEATS      8
#define GVEH_TIRE_LUT       16
#define GVEH_MAX_SURFACES   16
#define GVEH_MAX_AIR_STORES  8
#define GVEH_MAX_TANK_AMMO   7
#define GVEH_MAX_TANK_MODULES 9

#define GVEH_TRUE           1
#define GVEH_FALSE          0

#define GVEH_AXIS_X         0
#define GVEH_AXIS_Y         1
#define GVEH_AXIS_Z         2

#endif
