#ifndef GVEH_CONFIG_H
#define GVEH_CONFIG_H

/*
   gvehicle89 v6 public build knobs.
   C89, fixed point, no heap. Defaults target Win32/MinGW32 and common 32-bit int ABIs.
   If a very old/compiler-specific platform needs another 32-bit type, define
   GVEH_I32_TYPE/GVEH_U32_TYPE before including gveh.h.
*/
#ifndef GVEH_I32_TYPE
#define GVEH_I32_TYPE signed int
#endif
#ifndef GVEH_U32_TYPE
#define GVEH_U32_TYPE unsigned int
#endif

#ifndef GVEH_MAX_VEHICLES
#define GVEH_MAX_VEHICLES 16
#endif

#ifndef GVEH_COLLISION_ITERATIONS
#define GVEH_COLLISION_ITERATIONS 2
#endif

#ifndef GVEH_ENABLE_MODULE_CAR
#define GVEH_ENABLE_MODULE_CAR 1
#endif
#ifndef GVEH_ENABLE_MODULE_WATER
#define GVEH_ENABLE_MODULE_WATER 1
#endif
#ifndef GVEH_ENABLE_MODULE_AIR
#define GVEH_ENABLE_MODULE_AIR 1
#endif
#ifndef GVEH_ENABLE_MODULE_SPACE
#define GVEH_ENABLE_MODULE_SPACE 1
#endif
#ifndef GVEH_ENABLE_MODULE_TANK
#define GVEH_ENABLE_MODULE_TANK 1
#endif
#ifndef GVEH_ENABLE_MODULE_ARCADE
#define GVEH_ENABLE_MODULE_ARCADE 1
#endif

#endif
