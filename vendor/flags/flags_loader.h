/* flags_loader.h - declarative loader/watcher (poll-by-caller) - C89
 *
 * Rough C89 equivalent of flags_loader.py, but without threads:
 * - call flags_loader_load_once() periodically from your main loop.
 */
#ifndef FLAGS_LOADER_H
#define FLAGS_LOADER_H

#include "flags_state.h"

typedef struct FlagsLoader {
    FlagStore *flagstore; /* where direct "key=value" assignments go */
    FlagStore *mem;       /* where "+=" deltas apply (may be NULL) */

    const char *prefix_for_adds; /* optional */
    int has_lo;
    flags_fx_t lo;
    int has_hi;
    flags_fx_t hi;

    const FlagsKeyMap *target_map;
    int target_map_count;

    FlagState state;
} FlagsLoader;

/* Initialize loader and its internal FlagState (caller provides state storage). */
void flags_loader_init(FlagsLoader *ldr,
                       const char *filepath,
                       FlagStore *flagstore,
                       FlagStore *mem_for_adds,
                       const char *prefix_for_adds,
                       int has_lo, flags_fx_t lo,
                       int has_hi, flags_fx_t hi,
                       const FlagsKeyMap *target_map, int target_map_count,
                       FlagsKV *state_items, int state_items_cap,
                       char *state_pool_buf, int state_pool_cap);

/* Load+apply if changed. Return same as flagstate_load(). */
int flags_loader_load_once(FlagsLoader *ldr,
                           const FlagsIO *io,
                           char *file_buf, int file_buf_cap,
                           FlagsLogFn log_fn, void *log_user);

#endif /* FLAGS_LOADER_H */
