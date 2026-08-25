#ifndef GCROSSHAIR_RECIPE89_H
#define GCROSSHAIR_RECIPE89_H

#include "gcrosshair89_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GCB89_RECIPE_MAX_PRESETS 512
#define GCB89_RECIPE_MAX_ASSETS 128
#define GCB89_RECIPE_MAX_PATH 260
#define GCB89_RECIPE_MAX_NAME 64
#define GCB89_RECIPE_MAX_CATEGORY 32
#define GCB89_RECIPE_MAX_INCLUDE_DEPTH 8

typedef void *(*GCB89_RecipeOpenReadFn)(void *user, const char *path);
typedef int (*GCB89_RecipeReadLineFn)(void *user,
                                      void *handle,
                                      char *buffer,
                                      int capacity);
typedef void (*GCB89_RecipeCloseFn)(void *user, void *handle);

typedef struct GCB89_RecipeIoProvider {
    GCB89_RecipeOpenReadFn open_read;
    GCB89_RecipeReadLineFn read_line;
    GCB89_RecipeCloseFn close;
} GCB89_RecipeIoProvider;

/*
 * Optional VFS/PAK/ROM/embedded-filesystem bridge. If no provider is bound,
 * the recipe loader uses normal C stdio. Provider configuration survives
 * gcb89_recipe_reset() so it can be installed once before loading roots.
 */
void gcb89_recipe_set_io_provider(const GCB89_RecipeIoProvider *provider,
                                  void *user);
void gcb89_recipe_clear_io_provider(void);


/*
 * Loads the root INI. The root points to one or more list INIs; each list
 * points to preset INIs. Preset INIs may include other INIs as reusable parts.
 * All storage is static/fixed-capacity; the loader never calls malloc/realloc/free.
 */
int gcb89_recipe_load_root(const char *root_ini);
void gcb89_recipe_reset(void);
int gcb89_recipe_is_loaded(void);
const char *gcb89_recipe_last_error(void);
const char *gcb89_recipe_root_path(void);

#ifdef __cplusplus
}
#endif

#endif
