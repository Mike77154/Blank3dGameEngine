#ifndef PDC3D_CONFIG_H
#define PDC3D_CONFIG_H

/*
    PhysicalDamageCollision3D configuration.
    C89, fixed-point, no malloc/calloc/realloc/free, no float/double.

    The facade uses the hurtbox/melee fixed-point scale by default.
    Vendor modules keep their own prefixes internally:
      hb3/hit3/ml3:  HB3_FX_SHIFT, default 4
      g3d/t3d/w3d:   shift 8 in their original modules
*/

#ifndef PDC3D_MAX_EVENTS
#define PDC3D_MAX_EVENTS 256
#endif

#ifndef PDC3D_MAX_MATERIAL_RULES
#define PDC3D_MAX_MATERIAL_RULES 32
#endif

#ifndef PDC3D_MAX_PROFILE_HURTS
#define PDC3D_MAX_PROFILE_HURTS 16
#endif

#ifndef PDC3D_MAX_PROFILE_HITS
#define PDC3D_MAX_PROFILE_HITS 8
#endif

#ifndef PDC3D_ARENA_ALIGN
#define PDC3D_ARENA_ALIGN 4
#endif

#ifndef PDC3D_MAX_ACTOR_POSES
#define PDC3D_MAX_ACTOR_POSES 128
#endif

#ifndef PDC3D_ENABLE_DAMAGE_CALLBACK
#define PDC3D_ENABLE_DAMAGE_CALLBACK 0
#endif

#define PDC3D_VERSION_MAJOR 3
#define PDC3D_VERSION_MINOR 1
#define PDC3D_VERSION_PATCH 0

#endif
