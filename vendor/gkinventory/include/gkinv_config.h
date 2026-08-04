#ifndef GKINV_CONFIG_H
#define GKINV_CONFIG_H

/*
   GiffyKit Inventory configuration.

   This library is deliberately C89/C90 friendly:
   - no malloc/free/realloc
   - no C99 fixed-width headers
   - no bool
   - no real-number primitive types in the core
   - no variable length arrays

   Override these macros from your build system or before including gkinv.h.
*/

#ifndef GKINV_EMPTY_ITEM_ID
#define GKINV_EMPTY_ITEM_ID 0u
#endif

#ifndef GKINV_NO_INDEX
#define GKINV_NO_INDEX 65535u
#endif

/* Max slots that can be simulated on the stack for all-or-nothing ops. */
#ifndef GKINV_SIM_MAX_SLOTS
#define GKINV_SIM_MAX_SLOTS 128u
#endif

/* Max grid cells used by temporary occupancy maps. */
#ifndef GKINV_GRID_MAX_CELLS
#define GKINV_GRID_MAX_CELLS 256u
#endif

/* Fixed point defaults: signed Q16.16 stored in a signed long. */
#ifndef GKINV_FX_SHIFT
#define GKINV_FX_SHIFT 16
#endif

#ifndef GKINV_ENABLE_TEXT_IO
#define GKINV_ENABLE_TEXT_IO 1
#endif

#ifndef GKINV_ENABLE_RECIPES
#define GKINV_ENABLE_RECIPES 1
#endif

#ifndef GKINV_ENABLE_GRID
#define GKINV_ENABLE_GRID 1
#endif

#ifndef GKINV_ENABLE_HOOKS
#define GKINV_ENABLE_HOOKS 1
#endif

#endif
