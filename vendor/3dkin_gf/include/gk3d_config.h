#ifndef GK3D_CONFIG_H
#define GK3D_CONFIG_H

/*
    3DKin-GF configuration.
    Override these macros before including gk3d.h.
    C89, fixed point, static pools, no heap allocation.
*/

#ifndef GK3D_VERSION_MAJOR
#define GK3D_VERSION_MAJOR 0
#endif

#ifndef GK3D_VERSION_MINOR
#define GK3D_VERSION_MINOR 1
#endif

#ifndef GK3D_VERSION_PATCH
#define GK3D_VERSION_PATCH 0
#endif

#ifndef GK3D_VERSION_STRING
#define GK3D_VERSION_STRING "0.1.0"
#endif

#ifndef GK3D_MAX_OBJECTS
#define GK3D_MAX_OBJECTS 128
#endif

#ifndef GK3D_FIX_SHIFT
#define GK3D_FIX_SHIFT 8
#endif

#ifndef GK3D_DEFAULT_PROBE_RAW
#define GK3D_DEFAULT_PROBE_RAW 1L
#endif

#ifndef GK3D_DEFAULT_SOLVER_STEP_PIXELS
#define GK3D_DEFAULT_SOLVER_STEP_PIXELS 1
#endif

#ifndef GK3D_SOLVER_MAX_STEPS
#define GK3D_SOLVER_MAX_STEPS 256
#endif

#ifndef GK3D_SOLVER_BINARY_STEPS
#define GK3D_SOLVER_BINARY_STEPS 12
#endif

#endif
