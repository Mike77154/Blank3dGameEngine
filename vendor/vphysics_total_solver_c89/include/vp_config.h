#ifndef VP_CONFIG_H
#define VP_CONFIG_H

/*
  vphyslite "PRO" config knobs
  ---------------------------
  - Still C89, fixed-point, no malloc.
  - You can override any of these with -D macros.
*/

#ifndef VP_FRAC_BITS
#define VP_FRAC_BITS 16
#endif

/* default capacities (only used if you don't set them in vpWorldDesc) */
#ifndef VP_MAX_BODIES
#define VP_MAX_BODIES 256
#endif
#ifndef VP_MAX_CONTACTS
#define VP_MAX_CONTACTS 1024
#endif
#ifndef VP_MAX_JOINTS
#define VP_MAX_JOINTS 768
#endif

/* warm-start cache for contact impulses (must be power-of-two) */
#ifndef VP_CONTACT_CACHE_SIZE
#define VP_CONTACT_CACHE_SIZE 4096
#endif

/* pair cache for manifolds + contact begin/end events (must be power-of-two) */
#ifndef VP_PAIR_CACHE_SIZE
#define VP_PAIR_CACHE_SIZE 2048
#endif

/* max raw candidates stored per pair before reduction */
#ifndef VP_MANIFOLD_CANDIDATES_MAX
#define VP_MANIFOLD_CANDIDATES_MAX 16
#endif

/* manifold points kept per pair */
#ifndef VP_MANIFOLD_MAX_POINTS
#define VP_MANIFOLD_MAX_POINTS 4
#endif

/* local-space matching tolerance for reusing manifold point keys */
#ifndef VP_MANIFOLD_MATCH_DIST
#define VP_MANIFOLD_MATCH_DIST (1 << (VP_FRAC_BITS - 6)) /* ~0.0156 */
#endif

/* event queue (begin/persist/end, sleep/wake, joint break) */
#ifndef VP_EVENT_QUEUE_SIZE
#define VP_EVENT_QUEUE_SIZE 1024
#endif

/* core solver */
#ifndef VP_SOLVER_ITERS
#define VP_SOLVER_ITERS 18
#endif

/* Baumgarte beta = VP_BETA_NUM / VP_BETA_DEN */
#ifndef VP_BETA_NUM
#define VP_BETA_NUM 1
#endif
#ifndef VP_BETA_DEN
#define VP_BETA_DEN 5
#endif

#ifndef VP_PENETRATION_SLOP
#define VP_PENETRATION_SLOP (1 << (VP_FRAC_BITS - 8))
#endif
#ifndef VP_RESTITUTION_VEL_THRESHOLD
#define VP_RESTITUTION_VEL_THRESHOLD (1 << (VP_FRAC_BITS - 1))
#endif

/* sleep */
#ifndef VP_SLEEP_LIN_THRESHOLD
#define VP_SLEEP_LIN_THRESHOLD (1 << (VP_FRAC_BITS - 8))
#endif
#ifndef VP_SLEEP_ANG_THRESHOLD
#define VP_SLEEP_ANG_THRESHOLD (1 << (VP_FRAC_BITS - 7))
#endif
#ifndef VP_SLEEP_FRAMES
#define VP_SLEEP_FRAMES 30
#endif

/* water */
#ifndef VP_WATER_DENSITY_DEFAULT
#define VP_WATER_DENSITY_DEFAULT (1 << VP_FRAC_BITS)
#endif
#ifndef VP_WATER_LINEAR_DRAG_DEFAULT
#define VP_WATER_LINEAR_DRAG_DEFAULT (1 << (VP_FRAC_BITS - 2))
#endif
#ifndef VP_WATER_ANGULAR_DRAG_DEFAULT
#define VP_WATER_ANGULAR_DRAG_DEFAULT (1 << (VP_FRAC_BITS - 2))
#endif

/* damping */
#ifndef VP_DEFAULT_LINEAR_DAMP
#define VP_DEFAULT_LINEAR_DAMP (1 << (VP_FRAC_BITS - 5)) /* 0.03125 */
#endif
#ifndef VP_DEFAULT_ANGULAR_DAMP
#define VP_DEFAULT_ANGULAR_DAMP (1 << (VP_FRAC_BITS - 5))
#endif

/* physgun defaults */
#ifndef VP_PHYSGUN_STIFFNESS_DEFAULT
#define VP_PHYSGUN_STIFFNESS_DEFAULT (10 << VP_FRAC_BITS)
#endif
#ifndef VP_PHYSGUN_DAMPING_DEFAULT
#define VP_PHYSGUN_DAMPING_DEFAULT (3 << VP_FRAC_BITS)
#endif
#ifndef VP_PHYSGUN_MAX_FORCE_DEFAULT
#define VP_PHYSGUN_MAX_FORCE_DEFAULT (40 << VP_FRAC_BITS)
#endif

/* arena alignment used by every internal segment */
#ifndef VP_ARENA_ALIGNMENT
#define VP_ARENA_ALIGNMENT 8
#endif
#if (VP_ARENA_ALIGNMENT < 1) || ((VP_ARENA_ALIGNMENT & (VP_ARENA_ALIGNMENT - 1)) != 0)
# error "VP_ARENA_ALIGNMENT must be a power of two"
#endif

/* --- feature toggles --- */
#ifndef VP_ENABLE_MANIFOLDS
#define VP_ENABLE_MANIFOLDS 1
#endif

#ifndef VP_ENABLE_ISLANDS
#define VP_ENABLE_ISLANDS 1
#endif

#ifndef VP_ENABLE_SPLIT_IMPULSE
#define VP_ENABLE_SPLIT_IMPULSE 1
#endif

/* clamp for bias velocities (split impulse); 0 disables clamp */
#ifndef VP_SPLIT_IMPULSE_MAX
#define VP_SPLIT_IMPULSE_MAX (4 << VP_FRAC_BITS)
#endif

#endif /* VP_CONFIG_H */
