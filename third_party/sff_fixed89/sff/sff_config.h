/* sff_config.h - compile-time capacities for no-heap core */
#ifndef SFF_CONFIG_H
#define SFF_CONFIG_H

#ifndef SFF_MAX_SPRITES
#define SFF_MAX_SPRITES      8192u
#endif

#ifndef SFF_MAX_PALETTES
#define SFF_MAX_PALETTES      512u
#endif

/* Must be a power of two. Keep at least >= 2 * SFF_MAX_SPRITES for good probe behavior. */
#ifndef SFF_MAP_CAPACITY
#define SFF_MAP_CAPACITY    16384u
#endif

#ifndef SFF_INDEX0_TRANSPARENT
#define SFF_INDEX0_TRANSPARENT 1
#endif

#endif /* SFF_CONFIG_H */
